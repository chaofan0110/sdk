/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  telnetd.c
 *
 *  Created on: 2014-11-3
 *      Author: Larry
 *        Todo: 提供telnet服务
 *     History:
 */


#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>

#if INCLUDE_DEBUG_CLI_SERVER

/* include sys header files */
#include <pthread.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <netinet/in.h>

/* include private header files */
#include "telnetd.h"
#include "../cli/cli_server.h"

/* 定义最大接收缓存，用在命令行接收输入时 */
#define MAX_RECV_BUFF_LENGTH    512

/* 最大telnet客户端连接数 */
#define MAX_CLIENT_NUMBER       DOS_CLIENT_MAX_NUM

/* 定义telnetserver监听端口 */
#define TELNETD_LISTEN_PORT     DOS_TELNETD_LINSTEN_PORT

/* 特殊按键ascii序列 */
#define TELNETD_SPECIAL_KEY_LEN          8

/* 定义两个常用按键 */
#define ESC_ASCII_CODE                   27  /* ESC Key */
#define DEL_ASCII_CODE                   127 /* DEL Key */
#define TAB_ASCII_CODE                   9   /* TAB Key */

/**
 * 特殊按键枚举
 */
enum KEY_LIST{
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,

    KEY_UP,
    KEY_RIGHT,
    KEY_DOWN,
    KEY_LEFT,

    KEY_BUTT
};

/**
 * telnet读取时的错误码
 */
enum TELNET_RECV_RESULT{
    TELNET_RECV_RESULT_ERROR = -1,
    TELNET_RECV_RESULT_TAB,
    TELNET_RECV_RESULT_HOT_KEY,
    TELNET_RECV_RESULT_NORMAL,

    TELNET_RECV_RESULT_BUTT
};

/**
 * telnet客户端管理控制块
 */
typedef struct tagTelnetClinetInfo{
    U32    ulIndex;     /* 和其他模块通讯使用的编号 */

    U32    ulValid;      /* 是否启用 */
    S32    lSocket;     /* 客户端socket */
    FILE   *pFDInput;   /* 输入描述符 */
    FILE   *pFDOutput;  /* 输出描述符 */

    U32    ulMode;      /* 但前客户端处于什么模式 */

    pthread_t pthClientTask; /* 线程id */
}TELNET_CLIENT_INFO_ST;

/**
 * 特殊按键时需要缓存，缓存结构体
 */
typedef struct tagKeyLisyBuff{
    U32 ulLength;
    S8  szKeyBuff[TELNETD_SPECIAL_KEY_LEN];
}KEY_LIST_BUFF_ST;

/* 模块内部的全局变量 */
/* 对客户端控制块进行保护 */
static pthread_mutex_t g_TelnetdMutex = PTHREAD_MUTEX_INITIALIZER;

/* 服务器线程ID */
static pthread_t g_pthTelnetdTask;

/* 等待退出标志 */
static S32 g_lTelnetdWaitingExit = 0;

/* telnet服务器监听socket */
static S32 g_lTelnetdSrvSocket = -1;

/* 客户端控制块列表 */
static TELNET_CLIENT_INFO_ST *g_pstTelnetClientList[MAX_CLIENT_NUMBER];

/*
 * These are the options we want to use as
 * a telnet server. These are set in telnetd_set_options2client()
 */
static U8 g_aucTelnetOptions[TELNET_MAX_OPTIONS] = { 0 };
static U8 g_taucTelnetWillack[TELNET_MAX_OPTIONS] = { 0 };

/*
 * These are the values we have set or
 * agreed to during our handshake.
 * These are set in telnetd_send_cmd2client(...)
 */
static U8 g_aucTelnetDoSet[TELNET_MAX_OPTIONS]  = { 0 };
static U8 g_aucTelnetWillSet[TELNET_MAX_OPTIONS]= { 0 };

/*
 * 特殊按键定义,
 *
 * 1.请不要打乱该数组的顺序；
 * 2.请保持该数组顺序与KEY_LIST一致
 */
static const S8 g_szSpecialKey[][TELNETD_SPECIAL_KEY_LEN] = {
        {ESC_ASCII_CODE, 'O', 'P', '\0', },   /* F1 */
        {ESC_ASCII_CODE, 'O', 'Q', '\0', },   /* F2 */
        {ESC_ASCII_CODE, 'O', 'R', '\0', },   /* F3 */
        {ESC_ASCII_CODE, 'O', 'S', '\0', },   /* F4 */
        {ESC_ASCII_CODE, '[', '1', '5' , DEL_ASCII_CODE, '\0', },   /* F5 */
        {ESC_ASCII_CODE, '[', '1', '7' , DEL_ASCII_CODE, '\0', },   /* F6 */
        {ESC_ASCII_CODE, '[', '1', '8' , DEL_ASCII_CODE, '\0', },   /* F7 */
        {ESC_ASCII_CODE, '[', '1', '9' , DEL_ASCII_CODE, '\0', },   /* F8 */
        {ESC_ASCII_CODE, '[', '2', '0' , DEL_ASCII_CODE, '\0', },   /* F9 */
        {ESC_ASCII_CODE, '[', '2', '1' , DEL_ASCII_CODE, '\0', },   /* F10 */

        {ESC_ASCII_CODE, '[', 'A', '\0', },   /* UP */
        {ESC_ASCII_CODE, '[', 'B', '\0', },   /* RIGHT */
        {ESC_ASCII_CODE, '[', 'C', '\0', },   /* DOWN */
        {ESC_ASCII_CODE, '[', 'D', '\0', },   /* LEFT */

        {'\0'}
};


/**
 * 函数：S32 telnet_set_mode(U32 ulIndex, U32 ulMode)
 * 功能：
 *      1.设置index所标示的客户端当前模式
 *      2.提供给外部模块是有
 * 参数
 *      U32 ulIndex：客户端ID
 *      U32 ulMode：模式
 * 返回值：成功返回0，失败返回－1
 */
S32 telnet_set_mode(U32 ulIndex, U32 ulMode)
{
    TELNET_CLIENT_INFO_ST *pstTelnetClient;

    if (ulIndex >= MAX_CLIENT_NUMBER)
    {
        logr_debug("Request telnet server send data to client, but given an invalid client index(%d).", ulIndex);
        DOS_ASSERT(0);
        return -1;
    }

    if (ulMode >= CLIENT_LEVEL_BUTT)
    {
        logr_debug("Request telnet server send data to client, but given an invalid mode (%d).", ulMode);
        DOS_ASSERT(0);
        return -1;
    }

    pstTelnetClient = g_pstTelnetClientList[ulIndex];
    if (!pstTelnetClient->ulValid)
    {
        logr_debug("Cannot find a valid client which have an index %d.", ulIndex);
        DOS_ASSERT(0);
        return -1;
    }

    pstTelnetClient->ulMode = ulMode;
    return 0;
}

/**
 * 函数：S32 telnet_out_string(U32 ulIndex, S8 *pszBuffer)
 * 功能：
 *      1.index所标示的客户端发送数据
 *      2.提供给外部模块是有
 * 参数
 *      U32 ulIndex：客户端ID
 *      S8 *pszBuffer：缓存
 * 返回值：成功返回0，失败返回－1
 */
S32 telnet_out_string(U32 ulIndex, S8 *pszBuffer)
{
    TELNET_CLIENT_INFO_ST *pstTelnetClient = NULL;

    if (ulIndex >= MAX_CLIENT_NUMBER)
    {
        logr_debug("Request telnet server send data to client, but given an invalid client index(%d).", ulIndex);
        DOS_ASSERT(0);
        return -1;
    }

    if (!pszBuffer)
    {
        logr_debug("%s", "Request telnet server send data to client, but given an empty buffer.");
        DOS_ASSERT(0);
        return -1;
    }

    pstTelnetClient = g_pstTelnetClientList[ulIndex];
    if (!pstTelnetClient->ulValid)
    {
        logr_debug("Cannot find a valid client which have an index %d.", ulIndex);
        DOS_ASSERT(0);
        return -1;
    }

    if (pstTelnetClient->lSocket <= 0)
    {
        logr_debug("Client with the index %d have an invalid socket.", ulIndex);
        DOS_ASSERT(0);
        return -1;
    }

    fprintf(pstTelnetClient->pFDOutput, "%s", (S8 *)pszBuffer);
    fflush(pstTelnetClient->pFDOutput);

    logr_debug("Send data to client, client index:%d", ulIndex);

    return 0;
}

/**
 * 函数：S32 telnet_out_string(U32 ulIndex, S8 *pszBuffer)
 * 功能：
 *      1.index所标示的客户端发送数据
 *      2.提供给外部模块是有
 *      3.buffer需要携带结束符
 * 参数
 *      U32 ulIndex：客户端ID
 *      S8 *pszBuffer：缓存
 * 返回值：成功返回0，失败返回－1
 */
S32 telnet_send_data(U32 ulIndex, U32 ulType, S8 *pszBuffer, U32 ulLen)
{
    TELNET_CLIENT_INFO_ST *pstTelnetClient;
    U32 i;

    if (!pszBuffer || ulLen<=0)
    {
        logr_debug("%s", "Request telnet server send data to client, but given an empty buffer.");
        DOS_ASSERT(0);
        return -1;
    }

    /* 强制给结束符 */
    pszBuffer[ulLen] = '\0';

    if (MSG_TYPE_LOG == ulType)
    {
        logr_debug("%s", "Request telnet server send log to client.");
        goto broadcast;
    }

    if (ulIndex >= MAX_CLIENT_NUMBER)
    {
        logr_debug("Request telnet server send data to client, but given an invalid client index(%d).", ulIndex);
        DOS_ASSERT(0);
        return -1;
    }

    pstTelnetClient = g_pstTelnetClientList[ulIndex];
    if (!pstTelnetClient->ulValid)
    {
        logr_debug("Cannot find a valid client which have an index %d.", ulIndex);
        DOS_ASSERT(0);
        return -1;
    }

    if (pstTelnetClient->lSocket <= 0)
    {
        logr_debug("Client with the index %d have an invalid socket.", ulIndex);
        DOS_ASSERT(0);
        return -1;
    }

    fprintf(pstTelnetClient->pFDOutput, "%s", (S8 *)pszBuffer);
    fflush(pstTelnetClient->pFDOutput);

    logr_debug("Send data to client, client index:%d length:%d", ulIndex, ulLen);

    return 0;

broadcast:

    for (i=0; i<MAX_CLIENT_NUMBER; i++)
    {
        if (g_pstTelnetClientList[i]->ulValid
            && CLIENT_LEVEL_LOG == g_pstTelnetClientList[i]->ulMode)
        {
            fprintf(g_pstTelnetClientList[i]->pFDOutput, "%s", (S8 *)pszBuffer);
            fflush(g_pstTelnetClientList[i]->pFDOutput);
        }
    }

    logr_debug("Send data to client, client index:%d length:%d", ulIndex, ulLen);

    return 0;
}


/**
 * 函数：VOID telnet_opt_init()
 * 功能：初始化telnet协商相关数据
 * 参数：
 * 返回值：
 */
VOID telnetd_opt_init()
{
    dos_memzero(g_aucTelnetOptions, sizeof(g_aucTelnetOptions));
    dos_memzero(g_aucTelnetOptions, sizeof(g_aucTelnetOptions));

    dos_memzero(g_aucTelnetDoSet, sizeof(g_aucTelnetDoSet));
    dos_memzero(g_aucTelnetWillSet, sizeof(g_aucTelnetWillSet));
}


/**
 * 函数：VOID telnetd_send_cmd2client(FILE *output, S32 lCmd, S32 lOpt)
 * 功能：
 *      用于和客户端协商时向客户端发送命令字
 * 参数
 *      FILE *output: 输出描述符
 *      S32 lCmd：命令字
 *      S32 lOpt：命令值
 * 返回值：void
 */
VOID telnetd_send_cmd2client(FILE *output, S32 lCmd, S32 lOpt)
{
    if (!output)
    {
        DOS_ASSERT(0);
        return;
    }

    /* Send a command to the telnet client */
    if (lCmd == DO || lCmd == DONT)
    {
        /* DO commands say what the client should do. */
        if (((lCmd == DO) && (g_aucTelnetDoSet[lOpt] != DO)) || ((lCmd == DONT) && (g_aucTelnetDoSet[lCmd] != DONT)))
        {
            /* And we only send them if there is a disagreement */
            g_aucTelnetDoSet[lOpt] = lCmd;
            fprintf(output, "%c%c%c", IAC, lCmd, lOpt);
        }
    }
    else if (lCmd == WILL || lCmd == WONT)
    {
        /* Similarly, WILL commands say what the server will do. */
        if (((lCmd == WILL) && (g_aucTelnetWillSet[lOpt] != WILL)) || ((lCmd == WONT) && (g_aucTelnetWillSet[lOpt] != WONT)))
        {
            /* And we only send them during disagreements */
            g_aucTelnetWillSet[lOpt] = lCmd;
            fprintf(output, "%c%c%c", IAC, lCmd, lOpt);
        }
    }
    else
    {
        /* Other commands are sent raw */
        fprintf(output, "%c%c", IAC, lCmd);
    }
    fflush(output);
}

/*
 * Set the default options for the telnet server.
 */
VOID telnetd_set_options4client(FILE *output)
{
    S32 option;

    if (!output)
    {
        DOS_ASSERT(0);
        return;
    }

    /* We will echo input */
    g_aucTelnetOptions[ECHO] = WILL;
    /* We will set graphics modes */
    g_aucTelnetOptions[SGA] = WILL;
    /* We will not set new environments */
    g_aucTelnetOptions[NEW_ENVIRON] = WONT;

    /* The client should not echo its own input */
    g_taucTelnetWillack[ECHO] = DONT;
    /* The client can set a graphics mode */
    g_taucTelnetWillack[SGA] = DO;
    /* We do not care about window size updates */
    g_taucTelnetWillack[NAWS] = DONT;
    /* The client should tell us its terminal type (very important) */
    g_taucTelnetWillack[TTYPE] = DO;
    /* No linemode */
    g_taucTelnetWillack[LINEMODE] = DONT;
    /* And the client can set a new environment */
    g_taucTelnetWillack[NEW_ENVIRON] = DO;


    /* Let the client know what we're using */
    for (option = 0; option < sizeof(g_aucTelnetOptions); ++option)
    {
        if (g_aucTelnetOptions[option])
        {
            telnetd_send_cmd2client(output, g_aucTelnetOptions[option], option);
        }
    }
    for (option = 0; option < sizeof(g_taucTelnetWillack); ++option)
    {
        if (g_taucTelnetWillack[option])
        {
            telnetd_send_cmd2client(output, g_taucTelnetWillack[option], option);
        }
    }
}

/*
 * Negotiate the telnet options.
 */
VOID telentd_negotiate(FILE *output, FILE *input)
{
    /* The default terminal is ANSI */
    S8 szTerm[TELNET_NEGO_BUFF_LENGTH] = {'a','n','s','i', 0};
    /* Various pieces for the telnet communication */
    S8 szNegoResult[TELNET_NEGO_BUFF_LENGTH] = { 0 };
    S32 lDone = 0, lSBMode = 0, lDoEcho = 0, lSBLen = 0, lMaxNegoTime = 0;
    U8 opt, i;

    if (!output || !input)
    {
        DOS_ASSERT(0);
        return;
    }

    telnetd_opt_init();

    /* Set the default options. */
    telnetd_set_options4client(output);

    /* Let's do this */
    while (!feof(stdin) && lDone < 1)
    {
        if (lMaxNegoTime > TELNET_MAX_NEGO_TIME)
        {
            DOS_ASSERT(0);
            return;
        }

        /* Get either IAC (start command) or a regular character (break, unless in SB mode) */
        i = getc(input);
        if (IAC == i)
        {
            /* If IAC, get the command */
            i = getc(input);
            switch (i)
            {
                case SE:
                    /* End of extended option mode */
                    lSBMode = 0;
                    if (TTYPE == szNegoResult[0])
                    {
                        //alarm(0);
                        //is_telnet_client = 1;
                        /* This was a response to the TTYPE command, meaning
                         * that this should be a terminal type */
                        strncpy(szTerm, &szNegoResult[2], sizeof(szTerm) - 1);
                        szTerm[sizeof(szTerm) - 1] = 0;
                        ++lDone;
                    }
                    break;
                case NOP:
                    /* No Op */
                    telnetd_send_cmd2client(output, NOP, 0);
                    fflush(output);
                    break;
                case WILL:
                case WONT:
                    /* Will / Won't Negotiation */
                    opt = getc(input);
                    if (opt < 0 || opt >= sizeof(g_aucTelnetOptions))
                    {
                        DOS_ASSERT(0);
                        return;
                    }
                    if (!g_aucTelnetOptions[opt])
                    {
                        /* We default to WONT */
                        g_aucTelnetOptions[opt] = WONT;
                    }
                    telnetd_send_cmd2client(output, g_aucTelnetOptions[opt], opt);
                    fflush(output);
                    if ((WILL == i) && (TTYPE == opt))
                    {
                        /* WILL TTYPE? Great, let's do that now! */
                        fprintf(output, "%c%c%c%c%c%c", IAC, SB, TTYPE, SEND, IAC, SE);
                        fflush(output);
                    }
                    break;
                case DO:
                case DONT:
                    /* Do / Don't Negotiation */
                    opt = getc(input);
                    if (opt < 0 || opt >= sizeof(g_aucTelnetOptions))
                    {
                        DOS_ASSERT(0);
                        return;
                    }
                    if (!g_aucTelnetOptions[opt])
                    {
                        /* We default to DONT */
                        g_aucTelnetOptions[opt] = DONT;
                    }
                    telnetd_send_cmd2client(output, g_aucTelnetOptions[opt], opt);

                    if (opt == ECHO)
                    {
                        lDoEcho = (i == DO);
                        DOS_ASSERT(lDoEcho);
                    }
                    fflush(output);
                    break;
                case SB:
                    /* Begin Extended Option Mode */
                    lSBMode = 1;
                    lSBLen  = 0;
                    memset(szNegoResult, 0, sizeof(szNegoResult));
                    break;
                case IAC:
                    /* IAC IAC? That's probably not right. */
                    lDone = 2;
                    break;
                default:
                    break;
            }
        }
        else if (lSBMode)
        {
            /* Extended Option Mode -> Accept character */
            if (lSBLen < (sizeof(szNegoResult) - 1))
            {
                /* Append this character to the SB string,
                 * but only if it doesn't put us over
                 * our limit; honestly, we shouldn't hit
                 * the limit, as we're only collecting characters
                 * for a terminal type or window size, but better safe than
                 * sorry (and vulnerable).
                 */
                szNegoResult[lSBLen++] = i;
            }
        }
    }
    /* What shall we now do with term, ttype, do_echo, and terminal_width? */
}

/**
 * 函数：VOID telnetd_client_output_prompt(FILE *output, U32 ulMode)
 * 功能：
 *      用于向客户端输出命令提示符
 * 参数
 *      FILE *output: 输出描述符
 *      U32 ulMode:当前客户端所处的模式
 * 返回值：void
 */
VOID telnetd_client_output_prompt(FILE *output, U32 ulMode)
{
    if (CLIENT_LEVEL_LOG == ulMode)
    {
        fprintf(output, " #");
    }
    else
    {
        fprintf(output, " >");
    }
    fflush(output);
}


/**
 * 函数：VOID telnet_send_new_line(FILE* output, S32 ulLines)
 * 功能：向output描述符输出空行
 * 参数：
 *      FILE* output：指向telnet客户端的描述符
 *      S32 ulLines：需要输出几个空行
 * 返回值：
 */
VOID telnetd_client_send_new_line(FILE* output, S32 ulLines)
{
    S32 i;

    for (i=0; i<ulLines; i++)
    {
        /* Send the telnet newline sequence */
        putc('\r', output);
        putc(0, output);
        putc('\n', output);
    }
}
#if 0
S32 telnet_client_tab_proc(S8 *pszCurrCmd, FILE *pfOutput)
{

}

/**
 * 函数：telnet_client_special_key_proc
 * 功能：处理客户端输入的特殊按键，包括方向件，以及f1-f10
 * 参数：
 *      U32 ulKey： 特殊键枚举值
 *      FILE *pfOutput：telnet客户端输出流
 *      S8 *pszCurrCmd：
 *      S32 lMaxLen,
 *      S32 *plCurrent
 * 返回值：
 *      成功返回接收缓存的长度，失败返回－1
 */
S32 telnet_client_special_key_proc(U32 ulKey, FILE *pfOutput, S8 *pszCurrCmd, S32 lMaxLen, S32 *plCurrent)
{

}
#endif
/**
 * 函数：S32 telnet_read_line(FILE *input, FILE *output, S8 *szBuffer, S32 lSize, S32 lMask)
 * 功能：读取客户端输入，直到回车
 * 参数：
 *      FILE *input：读取描述符
 *      FILE *output：输出描述符
 *      S8 *szBuffer：接收缓存
 *      S32 lSize：缓存大小
 *      S32 lMask：是否需要掩盖回显
 * 返回值：
 *      成功返回接收缓存的长度，失败返回－1
 */
S32 telnetd_client_read_line(FILE *input, FILE *output, S8 *szBuffer, S32 lSize, S32 lMask)
{
    S32 lLength;
    U8  c;
    KEY_LIST_BUFF_ST stKeyList;
#if 0
    S32 lLoop, lSpecialKeyMatchRet;
#endif

    if (!input || ! output)
    {
        DOS_ASSERT(0);
        return TELNET_RECV_RESULT_ERROR;
    }

    if (!szBuffer || lSize <= 0)
    {
        DOS_ASSERT(0);
        return TELNET_RECV_RESULT_ERROR;
    }


#define check_fd(_fd) \
do{ \
    if (feof(_fd)) \
    { \
        DOS_ASSERT(0); \
        return TELNET_RECV_RESULT_ERROR; \
    }\
}while(0)

    /* We make sure to restore the cursor. */
    fprintf(output, "\033[?25h");
    check_fd(output);
    fflush(output);

    if (feof(input))
    {
        DOS_ASSERT(0);
        return TELNET_RECV_RESULT_ERROR;
    }

    stKeyList.ulLength = 0;
    stKeyList.szKeyBuff[0] = '\0';

    for (lLength=0; lLength<lSize-1; lLength++)
    {
        c = getc(input);
        check_fd(input);

#if 0
        if (TAB_ASCII_CODE == c)
        {
            szBuffer[lLength] = '\0';
            return TELNET_RECV_RESULT_TAB;
        }
        /* 本流程主要检查左右方向键 */
        else if (ESC_ASCII_CODE == c || 0 != stKeyList.ulLength)
        {
            stKeyList.szKeyBuff[stKeyList.ulLength] = c;
            stKeyList.ulLength++;
            stKeyList.szKeyBuff[stKeyList.ulLength] = '\0';

            /* 检查特殊按键 */
            lSpecialKeyMatchRet = 1;
            for (lLoop=0; lLoop<KEY_BUTT; lLoop++)
            {
                /* 制定长度比较，如果没有匹配成功，说明已经不是特殊按键了 */
                if (dos_strncmp(g_szSpecialKey[lLoop], stKeyList.szKeyBuff, stKeyList.ulLength) != 0)
                {
                    lSpecialKeyMatchRet = 0;
                    break;
                }

                /* 全比较，如果没有匹配成功，有可能时还没有收完，或者是别的特殊按键，就要继续查找 */
                if (dos_strcmp(g_szSpecialKey[lLoop], stKeyList.szKeyBuff) != 0)
                {
                    continue;
                }

                /* 全比较成功，匹配特殊按键成功 */
                while (lLength > 0)
                {
                    /* 光标后退一个，输出一个空格， 光标再次后退，实现删除字符的功能*/
                    fprintf(output, "\b \b");
                    check_fd(output);
                    fflush(output);
                    lLength--;
                }

                telnet_client_special_key_proc(lLoop, output, szBuffer, lSize, lLength);
                return TELNET_RECV_RESULT_AGAIN;
            }

            /* 成功就要处理特殊按键，失败要将所有缓存字符回显 */
            if (lSpecialKeyMatchRet)
            {
                continue;
            }
            else
            {
                for (lLoop=0; lLoop<stKeyList.ulLength; lLoop++)
                {
                    puts(stKeyList.szKeyBuff[lLoop], output);
                }

                stKeyList.ulLength = 0;
                stKeyList.szKeyBuff[0] = '\0';
            }

            continue;
        }
        else 
#endif
        if (c == '\r' || c == '\n')
        {
            if (c == '\r')
            {
            /* the next char is either \n or \0, which we can discard. */
                getc(input);
            }
            telnetd_client_send_new_line(output, 1);
            break;
        }
        else if (c == '\b' || c == 0x7f)
        {
            if (!lLength)
            {
                lLength = -1;
                continue;
            }
            if (lMask)
            {
                fprintf(output, "\033[%dD\033[K", lLength);
                check_fd(output);
                fflush(output);
                lLength = -1;
                continue;
            }
            else
            {
                fprintf(output, "\b \b");
                check_fd(output);
                fflush(output);
                lLength -= 2;
                continue;
            }
        }
        else if (c == 0xff || '\0' == c)
        {
            DOS_ASSERT(0);
            return TELNET_RECV_RESULT_ERROR;
        }
        else if (iscntrl(c))
        {
            --lLength;
            continue;
        }

        szBuffer[lLength] = c;
        putc(lMask ? '*' : c, output);
        check_fd(output);
        fflush(output);
    }
    szBuffer[lLength] = 0;

    /* And we hide it again at the end. */
    fprintf(output, "\033[?25l");
    check_fd(output);
    fflush(output);

    return TELNET_RECV_RESULT_NORMAL;
}

/**
 * 函数：TELNET_CLIENT_INFO_ST *telnetd_client_alloc(S32 lSock)
 * 功能：
 *      查找空闲控制块，初始化，然后返回
 * 参数:
 *      S32 lSock: 新客户端的socket
 * 返回值：成功返回控制块指针，失败返回0
 */
TELNET_CLIENT_INFO_ST *telnetd_client_alloc(S32 lSock)
{
    S32 i;

    for (i=0; i<MAX_CLIENT_NUMBER; i++)
    {
        if (!g_pstTelnetClientList[i]->ulValid)
        {
            break;
        }
    }

    if (i >= MAX_CLIENT_NUMBER)
    {
        logr_notice("%s", "Too many user.");
        DOS_ASSERT(0);
        return NULL;
    }

    g_pstTelnetClientList[i]->ulValid = DOS_TRUE;
    g_pstTelnetClientList[i]->lSocket = lSock;
    g_pstTelnetClientList[i]->pFDInput = NULL;
    g_pstTelnetClientList[i]->pFDOutput = NULL;
    g_pstTelnetClientList[i]->ulMode = CLIENT_LEVEL_CONFIG;

    return g_pstTelnetClientList[i];
}

/**
 * 函数：U32 telnetd_client_count()
 * 功能：
 *      统计激化状态的客户端控制块
 * 参数:
 * 返回值：成功返回激活状态控制块数量，失败返回0
 */
U32 telnetd_client_count()
{
    U32 ulCount = 0, i;

    for (i=0; i<MAX_CLIENT_NUMBER; i++)
    {
        if (g_pstTelnetClientList[i]->ulValid)
        {
            ulCount++;
        }
    }

    return ulCount;
}

/**
 * 函数：VOID *telnetd_client_task(VOID *ptr)
 * 功能：
 *      telnet客户端接收线程
 * 参数:
 * 返回值：
 */
VOID *telnetd_client_task(VOID *ptr)
{
    TELNET_CLIENT_INFO_ST *pstClientInfo;
    struct rlimit limit;
    S8 szRecvBuff[MAX_RECV_BUFF_LENGTH];
    S32 lLen;
    S8 szCmd[16] = { 0 };

    if (!ptr)
    {
        logr_warning("%s", "New telnet client thread with invalid param, exit.");
        DOS_ASSERT(0);
        pthread_exit(NULL);
    }

    pstClientInfo = (TELNET_CLIENT_INFO_ST *)ptr;

    /* 配置／协商telnet */
    limit.rlim_cur = limit.rlim_max = 90;
    setrlimit(RLIMIT_CPU, &limit);
    limit.rlim_cur = limit.rlim_max = 0;
    setrlimit(RLIMIT_NPROC, &limit);

    pstClientInfo->pFDInput = fdopen(pstClientInfo->lSocket, "r");
    if (!pstClientInfo->pFDInput) {
        logr_warning("%s", "Create input fd fail for the new telnet client.");
        DOS_ASSERT(0);
        pthread_exit(NULL);
    }
    pstClientInfo->pFDOutput = fdopen(pstClientInfo->lSocket, "w");
    if (!pstClientInfo->pFDOutput) {
        logr_warning("%s", "Create output fd fail for the new telnet client.");
        DOS_ASSERT(0);
        pthread_exit(NULL);
    }

    telentd_negotiate(pstClientInfo->pFDOutput, pstClientInfo->pFDInput);

    dos_printf("%s", "Telnet negotiate finished.");

    fprintf(pstClientInfo->pFDOutput, "Welcome Control Panel!");
    telnetd_client_send_new_line(pstClientInfo->pFDOutput, 1);
    fflush(pstClientInfo->pFDOutput);

    while (1)
    {
        /* 输出提示符 */
        telnetd_client_output_prompt(pstClientInfo->pFDOutput, pstClientInfo->ulMode);

        /* 读取命令行输入 */
        lLen = telnetd_client_read_line(pstClientInfo->pFDInput, pstClientInfo->pFDOutput, szRecvBuff, sizeof(szRecvBuff), 0);
        if (lLen < 0)
        {
            logo_notice("Telnetd", "Telnet Exit", DOS_TRUE, "%s", "Telnet client exit.");
            break;
        }

        if (0 == lLen)
        {
            continue;
        }

        /* 分析并执行命令 */
        cli_server_cmd_analyse(pstClientInfo->ulIndex, pstClientInfo->ulMode, szRecvBuff, lLen);
    }

    fclose(pstClientInfo->pFDOutput);
    fclose(pstClientInfo->pFDInput);
    close(pstClientInfo->lSocket);

    /* 重新初始化控制块 */
    pstClientInfo->pFDOutput = NULL;
    pstClientInfo->pFDInput = NULL;
    pstClientInfo->ulValid = DOS_FALSE;
    pstClientInfo->lSocket = -1;

    /* 如果所有客户端都关闭了，强制关闭控制台日志 */
    if (!telnetd_client_count())
    {
        snprintf(szCmd, sizeof(szCmd), "debug 0 ");
        cli_server_send_broadcast_cmd(INVALID_CLIENT_INDEX, szCmd, dos_strlen(szCmd) + 1);
    }

    return NULL;
}

/**
 * 函数：VOID *telnetd_main_loop(VOID *ptr)
 * 功能：
 *      telnet服务器监听线程
 * 参数:
 * 返回值：
 */
VOID *telnetd_main_loop(VOID *ptr)
{
    struct sockaddr_storage stAddr;
    TELNET_CLIENT_INFO_ST   *pstClientInfo = NULL;
    struct timeval           stTimeout={1, 0};
    socklen_t                ulLen = 0;
    fd_set                   stFDSet;
    S32                      lConnectSock = -1;
    S32                      lRet = 0, lMaxFd = 0, i;

    pthread_mutex_lock(&g_TelnetdMutex);
    g_lTelnetdWaitingExit = 0;
    pthread_mutex_unlock(&g_TelnetdMutex);

    while(1)
    {
        pthread_mutex_lock(&g_TelnetdMutex);
        if (g_lTelnetdWaitingExit)
        {
            logr_info("%d", "Telnetd waiting exit flag has been set. exit");
            pthread_mutex_unlock(&g_TelnetdMutex);
            break;
        }
        pthread_mutex_unlock(&g_TelnetdMutex);

        FD_ZERO(&stFDSet);
        FD_SET(g_lTelnetdSrvSocket, &stFDSet);
        lMaxFd = g_lTelnetdSrvSocket + 1;
        stTimeout.tv_sec = 1;
        stTimeout.tv_usec = 0;

        lRet = select(lMaxFd, &stFDSet, NULL, NULL, &stTimeout);
        if (lRet < 0)
        {
            logr_warning("%s", "Error happened when select return.");
            lConnectSock = -1;
            DOS_ASSERT(0);
            break;
        }
        else if (0 == lRet)
        {
            continue;
        }

        ulLen = sizeof(stAddr);
        lConnectSock = accept(g_lTelnetdSrvSocket, (struct sockaddr *)&stAddr, &ulLen);
        if (lConnectSock < 0)
        {
            logr_warning("Telnet server accept connection fail.(%d)", errno);
            DOS_ASSERT(0);
            break;
        }

        logr_notice("%s", "New telnet connection.");

        pthread_mutex_lock(&g_TelnetdMutex);
        pstClientInfo = telnetd_client_alloc(lConnectSock);
        if (!pstClientInfo)
        {
            send(lConnectSock, "Too many user.\r\n", sizeof("Too many user.\r\n"), 0);
            close(lConnectSock);

            pthread_mutex_unlock(&g_TelnetdMutex);
            continue;
        }
        pthread_mutex_unlock(&g_TelnetdMutex);

        lRet = pthread_create(&pstClientInfo->pthClientTask, 0, telnetd_client_task, pstClientInfo);
        if (lRet < 0)
        {
            DOS_ASSERT(0);
            logr_warning("%s", "Create thread for telnet client fail.");
            close(lConnectSock);
            continue;
        }
    }

    /* 释放所有客户端 */
    for (i=0; i<MAX_CLIENT_NUMBER; i++)
    {
        if (g_pstTelnetClientList[i]->ulValid)
        {
            pthread_cancel(g_pstTelnetClientList[i]->pthClientTask);

            if (g_pstTelnetClientList[i]->pFDInput)
            {
                fclose(g_pstTelnetClientList[i]->pFDInput);
            }

            if (g_pstTelnetClientList[i]->pFDOutput)
            {
                fclose(g_pstTelnetClientList[i]->pFDOutput);
            }

            if (g_pstTelnetClientList[i]->lSocket)
            {
                close(g_pstTelnetClientList[i]->lSocket);
            }
        }
    }

    if (g_lTelnetdSrvSocket > 0)
    {
        close(g_lTelnetdSrvSocket);
    }

    dos_printf("%s", "Telnetd exited.");

    return NULL;
}

/**
 * 函数：S32 telnetd_init()
 * 功能：
 *      初始化telnet服务器
 * 参数:
 * 返回值：
 */
S32 telnetd_init()
{
    S32 flag, i;
    struct sockaddr_in stListenAddr;
    TELNET_CLIENT_INFO_ST *pstClientCB;

    pstClientCB = (TELNET_CLIENT_INFO_ST *)dos_dmem_alloc(sizeof(TELNET_CLIENT_INFO_ST) * MAX_CLIENT_NUMBER);
    if (!pstClientCB)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_memzero((VOID*)pstClientCB, sizeof(TELNET_CLIENT_INFO_ST) * MAX_CLIENT_NUMBER);
    for (i=0; i<MAX_CLIENT_NUMBER; i++)
    {
        g_pstTelnetClientList[i] = pstClientCB + i;
        g_pstTelnetClientList[i]->ulValid = DOS_FALSE;
        g_pstTelnetClientList[i]->lSocket = -1;
        g_pstTelnetClientList[i]->ulIndex = i;
        g_pstTelnetClientList[i]->pFDInput = NULL;
        g_pstTelnetClientList[i]->pFDOutput = NULL;
        g_pstTelnetClientList[i]->ulMode = CLIENT_LEVEL_CONFIG;
    }

    /* We bind to port 23 before chrooting, as well. */
    g_lTelnetdSrvSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (g_lTelnetdSrvSocket < 0)
    {
        logr_warning("%s", "Create the telnetd server socket fail.");
        return DOS_FAIL;
    }
    flag = 1;
    setsockopt(g_lTelnetdSrvSocket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

    memset(&stListenAddr, 0, sizeof(stListenAddr));
    stListenAddr.sin_family = AF_INET;
    stListenAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    stListenAddr.sin_port = htons(TELNETD_LISTEN_PORT);
    if (bind(g_lTelnetdSrvSocket, (struct sockaddr *)&stListenAddr, sizeof(stListenAddr)) < 0)
    {
        logr_error("%s", "Telnet server bind IP/Address fail.");
        close(g_lTelnetdSrvSocket);
        g_lTelnetdSrvSocket = -1;
        return DOS_FAIL;
    }

    if (listen(g_lTelnetdSrvSocket, 5) < 0)
    {
        logr_error("Linsten on port %d fail", TELNETD_LISTEN_PORT);
        close(g_lTelnetdSrvSocket);
        g_lTelnetdSrvSocket = -1;
        return DOS_FAIL;
    }

    return 0;
}

/**
 * 函数：S32 telnetd_start()
 * 功能：
 *      启动telnet服务器
 * 参数:
 * 返回值：
 */
S32 telnetd_start()
{
    S32 lRet = 0;

    lRet = pthread_create(&g_pthTelnetdTask, 0, telnetd_main_loop, NULL);
    if (lRet < 0)
    {
        logr_warning("%s", "Create thread for telnetd fail.");
        close(g_lTelnetdSrvSocket);
        g_lTelnetdSrvSocket = -1;
        return DOS_FAIL;
    }

    dos_printf("%s", "telnet server start!");

    pthread_join(g_pthTelnetdTask, NULL);

    return DOS_SUCC;
}

/**
 * 函数：S32 telnetd_stop()
 * 功能：
 *      停止telnet服务器
 * 参数:
 * 返回值：
 */
S32 telnetd_stop()
{
    pthread_mutex_lock(&g_TelnetdMutex);
    g_lTelnetdWaitingExit = 1;
    pthread_mutex_unlock(&g_TelnetdMutex);

    pthread_join(g_pthTelnetdTask, NULL);

    return DOS_SUCC;
}

#else

S32 telnet_out_string(U32 ulIndex, S8 *pszBuffer)
{
    dos_printf("%s", pszBuffer);

    return 0;
}
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

