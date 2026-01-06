/*! 
 * POSIX replacement for HWSharedMemory (for non-Windows builds)
 */
#ifndef _HWMANAGER_SHAREDMEMORY_POSIX_H_
#define _HWMANAGER_SHAREDMEMORY_POSIX_H_

#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <chrono>
#include "HWLog.h"
#include "HWLock.h"
#include "HWMutex.h"

// Use project logger; test target will link HWLog.cpp so HWLOG is available.

// status enum for compatibility
enum HW_SHAREDSTATUS { HSS_NOTSHARED = 0, HSS_ASCREATOR = 1, HSS_ASUSER = 2 };

#define HWSM_REQUEST_MAX 256

static inline unsigned long portable_timeGetTime()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long)((ts.tv_sec * 1000ULL) + (ts.tv_nsec / 1000000ULL));
}

template<class EType> class HWSharedMemory
{
protected:
    std::wstring m_strMMFName;
    int m_shm_fd;
    size_t m_nMMFSize;
    char* m_pBuf;

    unsigned long* m_pSystemTime;
    unsigned long* m_pUserProcRequest;
    unsigned char* m_pRequest;
    EType* m_pShared;
    int m_eStatus;

    bool CreateMMF(bool bCreate = true)
    {
        if(m_strMMFName.size() == 0) { HWLog::Write((char*)"*** HWSharedMemory: No Shared Name"); return false; }
        DestroySharedMemory();

        HWMutex am((const wchar_t*)m_strMMFName.c_str());
        if(am.IsCreated() == false) { HWLog::Write((char*)"*** HWSharedMemory: Failed to create mutex"); return false; }

        int nMMFSize = sizeof(unsigned long) + sizeof(unsigned long) + HWSM_REQUEST_MAX + sizeof(EType) + 4;
        std::string name;
        for(wchar_t c: m_strMMFName) name.push_back((char)c);
        if(name.size() == 0) return false;
        std::string shmName = std::string("/") + name + "MMF";

        m_shm_fd = -1;
#if defined(__APPLE__)
        std::string tmpPath = std::string("/tmp/") + name + ".mmf";
        int openFlags = O_RDWR;
        if(bCreate) openFlags |= O_CREAT;
        m_shm_fd = open(tmpPath.c_str(), openFlags, 0666);
        if(m_shm_fd == -1)
        {
            HWLOG("HWSharedMemory: open('%s') failed (%s)", tmpPath.c_str(), strerror(errno));
            return false;
        }
        if(ftruncate(m_shm_fd, nMMFSize) != 0)
        {
            HWLOG("HWSharedMemory: ftruncate failed (%s)", strerror(errno));
            close(m_shm_fd);
            m_shm_fd = -1;
            return false;
        }
        void* p = mmap(NULL, nMMFSize, PROT_READ | PROT_WRITE, MAP_SHARED, m_shm_fd, 0);
        if(p == MAP_FAILED)
        {
            HWLOG("HWSharedMemory: mmap failed (%s)", strerror(errno));
            close(m_shm_fd);
            m_shm_fd = -1;
            return false;
        }
#else
        int shmFlags = O_RDWR;
        if(bCreate) shmFlags |= O_CREAT;
        m_shm_fd = shm_open(shmName.c_str(), shmFlags, 0666);
        if(m_shm_fd == -1)
        {
            HWLOG("HWSharedMemory: shm_open('%s') failed (%s)", shmName.c_str(), strerror(errno));
            return false;
        }
        if(ftruncate(m_shm_fd, nMMFSize) != 0)
        {
            HWLOG("HWSharedMemory: ftruncate failed (%s)", strerror(errno));
            close(m_shm_fd);
            m_shm_fd = -1;
            return false;
        }
        void* p = mmap(NULL, nMMFSize, PROT_READ | PROT_WRITE, MAP_SHARED, m_shm_fd, 0);
        if(p == MAP_FAILED)
        {
            HWLOG("HWSharedMemory: mmap failed (%s)", strerror(errno));
            close(m_shm_fd);
            m_shm_fd = -1;
            return false;
        }
#endif

        m_pBuf = (char*)p;
        m_nMMFSize = nMMFSize;

        m_pSystemTime = (unsigned long*)m_pBuf;
        m_pUserProcRequest = (unsigned long*)&m_pBuf[sizeof(unsigned long)];
        m_pRequest = (unsigned char*)&m_pBuf[sizeof(unsigned long) + sizeof(unsigned long)];
        m_pShared = (EType*)&m_pBuf[sizeof(unsigned long) + sizeof(unsigned long) + HWSM_REQUEST_MAX];

        return true;
    }

public:
    HWSharedMemory()
    {
        m_strMMFName.clear();
        m_shm_fd = -1;
        m_pBuf = nullptr;
        m_pSystemTime = nullptr;
        m_pUserProcRequest = nullptr;
        m_pRequest = nullptr;
        m_pShared = nullptr;
        m_eStatus = 0;
        m_nMMFSize = 0;
    }

    virtual ~HWSharedMemory() { DestroySharedMemory(); }

    bool IsShared()
    {
        if(m_eStatus == 0 || m_shm_fd < 0 || m_pShared == nullptr || m_pSystemTime == nullptr || m_pRequest == nullptr || m_pUserProcRequest == nullptr || m_strMMFName.size() == 0) return false;
        return true;
    }

    virtual bool CreateSharedMemory()
    {
        if(CreateMMF(true))
        {
            m_eStatus = 1; // creator
            if(m_pSystemTime) *m_pSystemTime = 0;
            if(m_pUserProcRequest) *m_pUserProcRequest = 0;
            if(m_pRequest) memset(m_pRequest, 0x00, HWSM_REQUEST_MAX);
            if(m_pShared) memset(m_pShared, 0x00, sizeof(EType));
            return true;
        }
        return false;
    }

    virtual bool OpenSharedMemory()
    {
        if(CreateMMF(false))
        {
            m_eStatus = 2; // user
            return true;
        }
        return false;
    }

    virtual void DestroySharedMemory()
    {
        if(m_strMMFName.size() == 0) return;
        HWMutex am((const wchar_t*)m_strMMFName.c_str());
        if(am.IsCreated() == false) return;

        if(m_pBuf)
        {
            munmap(m_pBuf, m_nMMFSize);
            m_pBuf = nullptr;
        }
        if(m_shm_fd >= 0)
        {
            close(m_shm_fd);
            m_shm_fd = -1;
        }

#if defined(__APPLE__)
        if(m_eStatus == 1)
        {
            std::string name;
            for(wchar_t c: m_strMMFName) name.push_back((char)c);
            if(name.size())
            {
                std::string tmpPath = std::string("/tmp/") + name + ".mmf";
                unlink(tmpPath.c_str());
            }
        }
#else
        if(m_eStatus == 1)
        {
            std::string name;
            for(wchar_t c: m_strMMFName) name.push_back((char)c);
            if(name.size())
            {
                std::string shmName = std::string("/") + name + "MMF";
                shm_unlink(shmName.c_str());
            }
        }
#endif

        m_pSystemTime = nullptr;
        m_pUserProcRequest = nullptr;
        m_pRequest = nullptr;
        m_pShared = nullptr;
        m_eStatus = 0;
    }

    unsigned long GetElapsedTimeFromLastUpdate()
    {
        HWMutex am((const wchar_t*)m_strMMFName.c_str());
        if(am.IsCreated() == false) return (unsigned long)UINT_MAX;
        return portable_timeGetTime() - *m_pSystemTime;
    }

    virtual bool IsDirty(bool& bRet)
    {
        HWMutex am((const wchar_t*)m_strMMFName.c_str());
        if(am.IsCreated() == false) { return false; }
        bRet = (*m_pUserProcRequest & 0x00000002) != 0 ? true:false;
        return true;
    }

    virtual bool IsRequestedReset(bool& bRet)
    {
        HWMutex am((const wchar_t*)m_strMMFName.c_str());
        if(am.IsCreated() == false) {return false;}
        bRet = (*m_pUserProcRequest & 0x00000004) != 0 ? true:false;
        return true;
    }

    virtual bool RequestReset()
    {
        HWMutex am((const wchar_t*)m_strMMFName.c_str());
        if(am.IsCreated() == false) {return false;}
        *m_pUserProcRequest = *m_pUserProcRequest | 0x00000004;
        return true;
    }

    virtual bool GetUserRequestCode(unsigned long& rRet)
    {
        HWMutex am((const wchar_t*)m_strMMFName.c_str());
        if(am.IsCreated() == false) {return false;}
        rRet = *m_pUserProcRequest;
        return true;
    }

    virtual bool SetUserRequestCode(unsigned long ulValue)
    {
        HWMutex am((const wchar_t*)m_strMMFName.c_str());
        if(am.IsCreated() == false) {return false;}
        *m_pUserProcRequest = ulValue;
        return true;
    }

    virtual bool CheckChange(int nIndex)
    {
        HWMutex am((const wchar_t*)m_strMMFName.c_str());
        if(am.IsCreated() == false) {return false;}
        if(m_pRequest == nullptr) return false;
        if(nIndex < 0 || nIndex >= HWSM_REQUEST_MAX) return false;
        return m_pRequest[nIndex] != 0 ? true:false;
    }

    virtual void ResetChange(int nIndex = INT_MAX)
    {
        HWMutex am((const wchar_t*)m_strMMFName.c_str());
        if(am.IsCreated() == false) {return;}
        if(m_pRequest == nullptr) return;
        if(nIndex == INT_MAX)
        {
            memset(m_pRequest, 0x00, HWSM_REQUEST_MAX);
        }
        else if(nIndex >= 0 && nIndex < HWSM_REQUEST_MAX)
        {
            m_pRequest[nIndex] = 0;
        }
    }

}; // class HWSharedMemory

#endif // _HWMANAGER_SHAREDMEMORY_POSIX_H_
