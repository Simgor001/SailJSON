#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
/*非数值类型是以json文本的形式保存。需要用到该数据的时候再解释。
优点是节省资源，缺点是导致使用数据时较慢*/
enum valType
{
    BOL,
    STR,
    ARR,
    OBJ,
    NUM,
    FLO

};
//键对象结构体
typedef struct jsonVal_tag
{
    char *key;                //键名
    enum valType valType;     //值类型
    void *valData;            //值数据指针
    struct jsonVal_tag *link; //上一条链表
} jsonVal;
typedef struct jsonArray_tag
{
    uint8_t length;             //数组长度
    struct jsonArray_tag *link; //数组数据头
} jsonArray;

int loadObject(const char *objectData, jsonVal **rootVal);
char *checkSign(const char *data, char sign);
char *getStr(const char *data, char **save);

int main(int argc, char const *argv[])
{
    char a[] = {"{\
    \"str\": \"control\",\
    \"float\": 12.3,\
    \"num\": 233,\
    \"bol\": true,\
    \"obj\": {\
    \"power_switch\": 1,\
    \"color\": 1,\
    \"brightness\": 66\
    }\
}"};

    jsonVal *rootVal;
    jsonVal *val;
    int length = 0;
    length = loadObject(a, &rootVal);
    val = rootVal;

    for (int i = 0; i < length; i++)
    {
        printf("\"%s\":", val->key);
        switch (val->valType)
        {
        case BOL:
        case NUM:
            printf("%d\n", val->valData);
            break;
        case FLO:
            printf("%f\n", *((float *)val->valData));
            break;
        case STR:
        case ARR:
        case OBJ:
            printf("%s\n", val->valData);
            break;
        }
        val = val->link;
    }

    return 0;
}

/*加载对象，并且返回链长度*/
int loadObject(const char *objectData, jsonVal **rootVal)
{
    uint8_t status = 0;
    uint8_t valCount = 0;
    jsonVal *val = 0;
    jsonVal *nextp = 0;
    char *pos = checkSign(objectData, '{');
    if (pos++ == NULL)
        //找不到对象
        return -1;
    do
    {
        valCount++;
        val = (jsonVal *)malloc(sizeof(jsonVal));
        pos = getStr(pos, &(val->key));
        if (pos == NULL)
            //找不到key
            return -2;

        pos = checkSign(pos, ':');

        if (pos++ == NULL)
            //找不到冒号
            return -3;

        //分析值
        char *sign[4] = {0};
        sign[0] = checkSign(pos, '"'); //引号位置
        sign[1] = checkSign(pos, '['); //中括号位置
        sign[2] = checkSign(pos, '{'); //大括号位置
        sign[3] = checkSign(pos, ','); //逗号位置

        char *signMin = sign[0]; //找最小值
        for (int i = 1; i < 4; i++)
        {
            if (sign[i] != NULL)
                signMin = min(sign[i], signMin);
        }

        int openCount = 0;
        int closeCount = 0;
        int length = 0;
        char *open;  //括号开始位置
        char *close; //符号结束位置
        char *data;
        switch (*(signMin))
        {
        case '"':
            //字符串
            pos = getStr(signMin, &val->valData);
            if (pos == 0)
                //找不到字符串
                return -4;
            val->valType = STR;
            break;
        case '[':
            //数组
            pos = signMin;
            do
            {
                open = checkSign(pos, '[');
                close = checkSign(pos, ']');
                if (close == NULL)
                    return -5;
                if (open < close && open != NULL)
                {
                    openCount++;
                    pos = open + 1;
                }
                else
                {
                    closeCount++;
                    pos = close + 1;
                }
            } while (openCount - closeCount > 0);
            length = pos - signMin;
            val->valData = (char *)malloc(length * sizeof(char));
            strncpy(val->valData, signMin, length);
            val->valType = ARR;
            break;
        case '{':
            //对象
            pos = signMin;
            do
            {
                open = checkSign(pos, '{');
                close = checkSign(pos, '}');
                if (close == NULL)
                    return -6;
                if (open < close && open != NULL)
                {
                    openCount++;
                    pos = open + 1;
                }
                else
                {
                    closeCount++;
                    pos = close + 1;
                }
            } while (openCount - closeCount > 0);
            length = pos - signMin;
            val->valData = (char *)malloc(length * sizeof(char));
            strncpy(val->valData, signMin, length);
            val->valType = OBJ;
            break;
        case ',':
            //数值
            length = signMin - pos;
            data = (char *)malloc(length * sizeof(char));
            strncpy(data, pos, length);
            if (strstr(data, "true"))
            {
                val->valData = 1;
                val->valType = BOL;
            }
            else if (strstr(data, "fasle"))
            {
                val->valData = 0;
                val->valType = BOL;
            }
            else if (strchr(data, '.'))
            {
                val->valData = (float *)malloc(sizeof(float));
                *((float *)val->valData) = atof(data);
                val->valType = FLO;
            }
            else
            {
                val->valData = atoi(data);
                val->valType = NUM;
            }

            break;

        default:
            status = 1;
            break;
        }
        pos = checkSign(pos, ',');
        if (pos == NULL)
            status = 2;
        val->link = nextp;
        nextp = val;

    } while (status == 0);
    *rootVal = val;

    return valCount;
}
char *checkSign(const char *data, char sign)
{
    //获取一个双引号位置
    char *qmarkAddress = strchr(data, '"');
    //获取一个符号的位置
    char *signAddress = strchr(data, sign);

    if (signAddress == NULL)
        return 0;
    else if (qmarkAddress == NULL && signAddress != NULL)
        return signAddress;
    else if (signAddress < qmarkAddress)
        //如果发现符号在双引号前面，就返回符号的位置
        return signAddress;
    else if (signAddress == qmarkAddress)
        return signAddress;
    else
    {
        //如果符号在双引号后面，首先要找到结束的引号，并且考虑转义符。
        do
        {
            qmarkAddress = strchr(qmarkAddress + 1, '"');
            if (qmarkAddress == NULL)
                break;
        } while (*(qmarkAddress - 1) == '\\');
        if (qmarkAddress == NULL)
            return 0;
        return strchr(qmarkAddress, sign);
    }
}
char *getStr(const char *data, char **save)
{
    //获取开双引号位置
    char *qmarkO = strchr(data, '"');
    if (qmarkO == NULL)
        return 0;

    char *qmarkC = qmarkO;
    do
    {
        qmarkC = strchr(qmarkC + 1, '"');
        if (qmarkC == NULL)
            break;
    } while (*(qmarkC - 1) == '\\');
    if (qmarkC == NULL)
        return 0;
    int length = qmarkC - qmarkO;
    char *address = malloc(length * sizeof(char));
    strncpy(address, qmarkO + 1, length - 1);
    *save = address;
    return qmarkC + 1;
}