#ifndef __LYW_CODE_HASH_HPP__
#define __LYW_CODE_HASH_HPP__
namespace LYW_CODE
{
    class Hash
    {
    public:
        static unsigned int APHash(const char *str, unsigned int strLen )
        {
            unsigned int hash = 0;
            int i;
            for (i = 0 ; i < strLen; i++)
            {
                if  ((i &  1 ) ==  0 )
                {
                    hash ^= ((hash << 7 ) ^ (str[i]) ^ (hash >>  3 ));
                }
                else
                {
                    hash ^= (~((hash << 11 ) ^ (str[i]) ^ (hash >> 5)));
                }
            }

            return  (hash & 0x7FFFFFFF );
        }
    };
}
#endif
