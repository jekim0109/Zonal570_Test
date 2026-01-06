#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include "common/HWManager/HWSharedMemory_posix.h"

struct TestData {
    int value;
    char pad[256];
};

class TestSHM : public HWSharedMemory<TestData> {
public:
    void SetName(const std::wstring &name) { m_strMMFName = name; }
    bool Create() { return CreateSharedMemory(); }
    bool Open() { return OpenSharedMemory(); }
    void SetValue(int v) { if(m_pShared) m_pShared->value = v; }
    int GetValue() { return m_pShared ? m_pShared->value : -1; }
};

int main() {
    TestSHM parent;
    parent.SetName(L"unit_test_shm");
    if(!parent.Create()) { std::cerr << "Parent: CreateSharedMemory failed\n"; return 1; }
    parent.SetValue(42);
    pid_t pid = fork();
    if(pid < 0) { perror("fork"); return 1; }
    if(pid == 0) {
        // child
        TestSHM child;
        child.SetName(L"unit_test_shm");
        if(!child.Open()) { std::cerr << "Child: OpenSharedMemory failed\n"; return 2; }
        int v = child.GetValue();
        std::cout << "CHILD read value=" << v << "\n";
        child.SetValue(99);
        std::cout << "CHILD wrote value=99\n";
        return 0;
    } else {
        int status = 0;
        waitpid(pid, &status, 0);
        int v = parent.GetValue();
        std::cout << "PARENT read after child value=" << v << "\n";
        parent.DestroySharedMemory();
    }
    return 0;
}
