#include "RWLock.hpp"

int main()
{

    LYW_CODE::RWDelaySafeLock m_lock;

    m_lock.WriteLock(1000);
    m_lock.WriteUnLock();


    int st = m_lock.ReadLock();
    m_lock.ReadUnLock(st);

    return 0;
}
