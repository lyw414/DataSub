namespace LYW_CODE
{
    int ProDataTransInit(key_t key);

    handle ProDataTransAllocateWB(int type, len);

    int ProDataTransWrite(handle h, void * data, int len, int index);

    int ProDataTransCommitWB(handle h, int type);

    int ProDataTransWrite(int type, void * data, int len);

    int ProDataTransRead(int type, function IsNeed);
}
