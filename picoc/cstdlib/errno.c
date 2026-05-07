/* ============================================================================
 * errno.c - PicoC 错误码定义 (errno.h)
 *
 * 为 PicoC 解释器提供 C 标准 <errno.h> 头文件中定义的系统错误码
 * 常量。通过 #ifdef 检查平台支持后注册所有可用的 POSIX 错误码。
 *
 * 错误码分类:
 *   - 文件访问错误:  EACCES / EPERM / ENOENT / EEXIST / ENOTDIR / EISDIR /
 *                    EROFS / ENOTEMPTY / EBADF / EIO / EMFILE / ENOSPC / ...
 *   - 内存管理错误:  ENOMEM / EAGAIN / EOVERFLOW / ERANGE / EDOM
 *   - 进程错误:      ECHILD / EDEADLK / EBUSY / EINTR / ESRCH / EINVAL /
 *                    ENOSYS / ENOTSUP / EPIPE / ...
 *   - 网络错误:      EADDRINUSE / ECONNREFUSED / ETIMEDOUT / EWOULDBLOCK / ...
 *
 * 除错误码常量外，还注册 errno 全局变量指向系统 errno。
 * 仅在未定义 BUILTIN_MINI_STDLIB 时编译。
 * ============================================================================ */

#include <errno.h>
#include "../interpreter.h"

#ifndef BUILTIN_MINI_STDLIB

/* =========================================================================
 * 文件访问错误码
 * ========================================================================= */

#ifdef EACCES
static int EACCESValue = EACCES;            /* 权限不足 */
#endif

#ifdef EPERM
static int EPERMValue = EPERM;              /* 操作不允许 */
#endif

#ifdef ENOENT
static int ENOENTValue = ENOENT;            /* 文件或目录不存在 */
#endif

#ifdef EEXIST
static int EEXISTValue = EEXIST;            /* 文件已存在 */
#endif

#ifdef ENOTDIR
static int ENOTDIRValue = ENOTDIR;          /* 路径分量不是目录 */
#endif

#ifdef EISDIR
static int EISDIRValue = EISDIR;            /* 操作目标是目录 */
#endif

#ifdef ELOOP
static int ELOOPValue = ELOOP;              /* 符号链接循环 */
#endif

#ifdef ENAMETOOLONG
static int ENAMETOOLONGValue = ENAMETOOLONG; /* 文件名过长 */
#endif

#ifdef EROFS
static int EROFSValue = EROFS;              /* 只读文件系统 */
#endif

#ifdef ENOTEMPTY
static int ENOTEMPTYValue = ENOTEMPTY;      /* 目录不为空 */
#endif

#ifdef EBADF
static int EBADFValue = EBADF;              /* 无效文件描述符 */
#endif

#ifdef EFAULT
static int EFAULTValue = EFAULT;            /* 地址错误 (bad address) */
#endif

#ifdef EFBIG
static int EFBIGValue = EFBIG;              /* 文件过大 */
#endif

#ifdef EIO
static int EIOValue = EIO;                  /* I/O 错误 */
#endif

#ifdef ENFILE
static int ENFILEValue = ENFILE;            /* 系统文件表溢出 */
#endif

#ifdef EMFILE
static int EMFILEValue = EMFILE;            /* 进程打开文件过多 */
#endif

#ifdef ENOSPC
static int ENOSPCValue = ENOSPC;            /* 设备空间不足 */
#endif

#ifdef ENXIO
static int ENXIOValue = ENXIO;              /* 设备或地址不存在 */
#endif

#ifdef ESPIPE
static int ESPIPEValue = ESPIPE;            /* 无效的管道定位操作 */
#endif

#ifdef EXDEV
static int EXDEVValue = EXDEV;              /* 跨设备链接 */
#endif

#ifdef ETXTBSY
static int ETXTBSYValue = ETXTBSY;          /* 文本文件正忙 */
#endif

#ifdef ENODEV
static int ENODEVValue = ENODEV;            /* 设备不存在 */
#endif

/* =========================================================================
 * 内存管理错误码
 * ========================================================================= */

#ifdef ENOMEM
static int ENOMEMValue = ENOMEM;            /* 内存不足 */
#endif

#ifdef EAGAIN
static int EAGAINValue = EAGAIN;            /* 资源暂时不可用 (需重试) */
#endif

#ifdef EOVERFLOW
static int EOVERFLOWValue = EOVERFLOW;      /* 数值溢出 */
#endif

#ifdef ERANGE
static int ERANGEValue = ERANGE;            /* 数学结果超出范围 */
#endif

#ifdef EDOM
static int EDOMValue = EDOM;                /* 数学参数超出定义域 */
#endif

/* =========================================================================
 * 进程和线程错误码
 * ========================================================================= */

#ifdef ECHILD
static int ECHILDValue = ECHILD;            /* 无子进程 */
#endif

#ifdef EDEADLK
static int EDEADLKValue = EDEADLK;          /* 资源死锁 */
#endif

#ifdef EBUSY
static int EBUSYValue = EBUSY;              /* 设备或资源忙 */
#endif

#ifdef EINTR
static int EINTRValue = EINTR;              /* 系统调用被信号中断 */
#endif

#ifdef ESRCH
static int ESRCHValue = ESRCH;              /* 无此进程 */
#endif

#ifdef EINVAL
static int EINVALValue = EINVAL;            /* 无效参数 */
#endif

#ifdef ENOSYS
static int ENOSYSValue = ENOSYS;            /* 功能未实现 */
#endif

#ifdef ENOTSUP
static int ENOTSUPValue = ENOTSUP;          /* 不支持的操作 */
#endif

#ifdef EOPNOTSUPP
static int EOPNOTSUPPValue = EOPNOTSUPP;    /* 操作不支持 */
#endif

#ifdef ENOTTY
static int ENOTTYValue = ENOTTY;            /* 不适当的 ioctl 设备 */
#endif

#ifdef ENOEXEC
static int ENOEXECValue = ENOEXEC;          /* 无效的可执行文件格式 */
#endif

#ifdef EMLINK
static int EMLINKValue = EMLINK;            /* 链接数过多 */
#endif

#ifdef EPIPE
static int EPIPEValue = EPIPE;              /* 管道破裂 (无读取端) */
#endif

#ifdef EINPROGRESS
static int EINPROGRESSValue = EINPROGRESS;  /* 操作进行中 */
#endif

#ifdef EALREADY
static int EALREADYValue = EALREADY;        /* 操作已在进行 */
#endif

#ifdef ECANCELED
static int ECANCELEDValue = ECANCELED;      /* 操作已取消 */
#endif

#ifdef ENOTSOCK
static int ENOTSOCKValue = ENOTSOCK;        /* 非套接字操作 */
#endif

#ifdef EDESTADDRREQ
static int EDESTADDRREQValue = EDESTADDRREQ; /* 需要目标地址 */
#endif

#ifdef EISCONN
static int EISCONNValue = EISCONN;          /* 传输端点已连接 */
#endif

#ifdef ENOTCONN
static int ENOTCONNValue = ENOTCONN;        /* 传输端点未连接 */
#endif

/* =========================================================================
 * 网络相关错误码
 * ========================================================================= */

#ifdef EADDRINUSE
static int EADDRINUSEValue = EADDRINUSE;    /* 地址已在使用 */
#endif

#ifdef EADDRNOTAVAIL
static int EADDRNOTAVAILValue = EADDRNOTAVAIL; /* 地址不可用 */
#endif

#ifdef EAFNOSUPPORT
static int EAFNOSUPPORTValue = EAFNOSUPPORT; /* 不支持的地址族 */
#endif

#ifdef ECONNABORTED
static int ECONNABORTEDValue = ECONNABORTED; /* 连接中止 */
#endif

#ifdef ECONNREFUSED
static int ECONNREFUSEDValue = ECONNREFUSED; /* 连接被拒绝 */
#endif

#ifdef ECONNRESET
static int ECONNRESETValue = ECONNRESET;    /* 连接被对端重置 */
#endif

#ifdef EHOSTUNREACH
static int EHOSTUNREACHValue = EHOSTUNREACH; /* 主机不可达 */
#endif

#ifdef ENETDOWN
static int ENETDOWNValue = ENETDOWN;        /* 网络不可用 */
#endif

#ifdef ENETRESET
static int ENETRESETValue = ENETRESET;      /* 网络连接被重置 */
#endif

#ifdef ENETUNREACH
static int ENETUNREACHValue = ENETUNREACH;  /* 网络不可达 */
#endif

#ifdef EMSGSIZE
static int EMSGSIZEValue = EMSGSIZE;        /* 消息过长 */
#endif

#ifdef ENOPROTOOPT
static int ENOPROTOOPTValue = ENOPROTOOPT;  /* 不支持的协议选项 */
#endif

#ifdef EPROTO
static int EPROTOValue = EPROTO;            /* 协议错误 */
#endif

#ifdef EPROTONOSUPPORT
static int EPROTONOSUPPORTValue = EPROTONOSUPPORT; /* 不支持的协议 */
#endif

#ifdef EPROTOTYPE
static int EPROTOTYPEValue = EPROTOTYPE;    /* 协议类型错误 */
#endif

#ifdef ETIMEDOUT
static int ETIMEDOUTValue = ETIMEDOUT;      /* 连接超时 */
#endif

#ifdef EWOULDBLOCK
static int EWOULDBLOCKValue = EWOULDBLOCK;  /* 非阻塞操作将阻塞 (同 EAGAIN) */
#endif

/* =========================================================================
 * 其他错误码
 * ========================================================================= */

#ifdef EBADMSG
static int EBADMSGValue = EBADMSG;          /* 坏消息 */
#endif

#ifdef EIDRM
static int EIDRMValue = EIDRM;              /* 标识符已移除 */
#endif

#ifdef ENOLCK
static int ENOLCKValue = ENOLCK;            /* 无可用锁 */
#endif

#ifdef ENOLINK
static int ENOLINKValue = ENOLINK;          /* 链路已断开 */
#endif

#ifdef ENOMSG
static int ENOMSGValue = ENOMSG;            /* 消息队列中无对应类型的消息 */
#endif

#ifdef ENODATA
static int ENODATAValue = ENODATA;          /* 无可用数据 */
#endif

#ifdef ENOSR
static int ENOSRValue = ENOSR;              /* 无流资源 */
#endif

#ifdef ENOSTR
static int ENOSTRValue = ENOSTR;            /* 非流设备 */
#endif

#ifdef ENOTRECOVERABLE
static int ENOTRECOVERABLEValue = ENOTRECOVERABLE; /* 不可恢复的错误 */
#endif

#ifdef EOWNERDEAD
static int EOWNERDEADValue = EOWNERDEAD;    /* 所有者已终止 */
#endif

#ifdef EDQUOT
static int EDQUOTValue = EDQUOT;            /* 磁盘配额超出 */
#endif

#ifdef EILSEQ
static int EILSEQValue = EILSEQ;            /* 非法的字节序列 */
#endif

#ifdef EMULTIHOP
static int EMULTIHOPValue = EMULTIHOP;      /* 多跳连接尝试 */
#endif

#ifdef ENOBUFS
static int ENOBUFSValue = ENOBUFS;          /* 无可用缓冲区空间 */
#endif

#ifdef ESTALE
static int ESTALEValue = ESTALE;            /* 陈旧的 NFS 文件句柄 */
#endif

#ifdef ETIME
static int ETIMEValue = ETIME;              /* 定时器超时 */
#endif


/* StdErrnoSetupFunc: 注册所有可用的 errno 常量和 errno 变量
 * 每个错误码通过 #ifdef 检查系统是否定义后才注册 */
void StdErrnoSetupFunc(Picoc *pc)
{
    /* 文件访问错误码 */
#ifdef EACCES
    VariableDefinePlatformVar(pc, NULL, "EACCES", &pc->IntType, (union AnyValue *)&EACCESValue, FALSE);
#endif

#ifdef EADDRINUSE
    VariableDefinePlatformVar(pc, NULL, "EADDRINUSE", &pc->IntType, (union AnyValue *)&EADDRINUSEValue, FALSE);
#endif

#ifdef EADDRNOTAVAIL
    VariableDefinePlatformVar(pc, NULL, "EADDRNOTAVAIL", &pc->IntType, (union AnyValue *)&EADDRNOTAVAILValue, FALSE);
#endif

#ifdef EAFNOSUPPORT
    VariableDefinePlatformVar(pc, NULL, "EAFNOSUPPORT", &pc->IntType, (union AnyValue *)&EAFNOSUPPORTValue, FALSE);
#endif

#ifdef EAGAIN
    VariableDefinePlatformVar(pc, NULL, "EAGAIN", &pc->IntType, (union AnyValue *)&EAGAINValue, FALSE);
#endif

#ifdef EALREADY
    VariableDefinePlatformVar(pc, NULL, "EALREADY", &pc->IntType, (union AnyValue *)&EALREADYValue, FALSE);
#endif

#ifdef EBADF
    VariableDefinePlatformVar(pc, NULL, "EBADF", &pc->IntType, (union AnyValue *)&EBADFValue, FALSE);
#endif

#ifdef EBADMSG
    VariableDefinePlatformVar(pc, NULL, "EBADMSG", &pc->IntType, (union AnyValue *)&EBADMSGValue, FALSE);
#endif

#ifdef EBUSY
    VariableDefinePlatformVar(pc, NULL, "EBUSY", &pc->IntType, (union AnyValue *)&EBUSYValue, FALSE);
#endif

#ifdef ECANCELED
    VariableDefinePlatformVar(pc, NULL, "ECANCELED", &pc->IntType, (union AnyValue *)&ECANCELEDValue, FALSE);
#endif

#ifdef ECHILD
    VariableDefinePlatformVar(pc, NULL, "ECHILD", &pc->IntType, (union AnyValue *)&ECHILDValue, FALSE);
#endif

#ifdef ECONNABORTED
    VariableDefinePlatformVar(pc, NULL, "ECONNABORTED", &pc->IntType, (union AnyValue *)&ECONNABORTEDValue, FALSE);
#endif

#ifdef ECONNREFUSED
    VariableDefinePlatformVar(pc, NULL, "ECONNREFUSED", &pc->IntType, (union AnyValue *)&ECONNREFUSEDValue, FALSE);
#endif

#ifdef ECONNRESET
    VariableDefinePlatformVar(pc, NULL, "ECONNRESET", &pc->IntType, (union AnyValue *)&ECONNRESETValue, FALSE);
#endif

#ifdef EDEADLK
    VariableDefinePlatformVar(pc, NULL, "EDEADLK", &pc->IntType, (union AnyValue *)&EDEADLKValue, FALSE);
#endif

#ifdef EDESTADDRREQ
    VariableDefinePlatformVar(pc, NULL, "EDESTADDRREQ", &pc->IntType, (union AnyValue *)&EDESTADDRREQValue, FALSE);
#endif

#ifdef EDOM
    VariableDefinePlatformVar(pc, NULL, "EDOM", &pc->IntType, (union AnyValue *)&EDOMValue, FALSE);
#endif

#ifdef EDQUOT
    VariableDefinePlatformVar(pc, NULL, "EDQUOT", &pc->IntType, (union AnyValue *)&EDQUOTValue, FALSE);
#endif

#ifdef EEXIST
    VariableDefinePlatformVar(pc, NULL, "EEXIST", &pc->IntType, (union AnyValue *)&EEXISTValue, FALSE);
#endif

#ifdef EFAULT
    VariableDefinePlatformVar(pc, NULL, "EFAULT", &pc->IntType, (union AnyValue *)&EFAULTValue, FALSE);
#endif

#ifdef EFBIG
    VariableDefinePlatformVar(pc, NULL, "EFBIG", &pc->IntType, (union AnyValue *)&EFBIGValue, FALSE);
#endif

#ifdef EHOSTUNREACH
    VariableDefinePlatformVar(pc, NULL, "EHOSTUNREACH", &pc->IntType, (union AnyValue *)&EHOSTUNREACHValue, FALSE);
#endif

#ifdef EIDRM
    VariableDefinePlatformVar(pc, NULL, "EIDRM", &pc->IntType, (union AnyValue *)&EIDRMValue, FALSE);
#endif

#ifdef EILSEQ
    VariableDefinePlatformVar(pc, NULL, "EILSEQ", &pc->IntType, (union AnyValue *)&EILSEQValue, FALSE);
#endif

#ifdef EINPROGRESS
    VariableDefinePlatformVar(pc, NULL, "EINPROGRESS", &pc->IntType, (union AnyValue *)&EINPROGRESSValue, FALSE);
#endif

#ifdef EINTR
    VariableDefinePlatformVar(pc, NULL, "EINTR", &pc->IntType, (union AnyValue *)&EINTRValue, FALSE);
#endif

#ifdef EINVAL
    VariableDefinePlatformVar(pc, NULL, "EINVAL", &pc->IntType, (union AnyValue *)&EINVALValue, FALSE);
#endif

#ifdef EIO
    VariableDefinePlatformVar(pc, NULL, "EIO", &pc->IntType, (union AnyValue *)&EIOValue, FALSE);
#endif

#ifdef EISCONN
    VariableDefinePlatformVar(pc, NULL, "EISCONN", &pc->IntType, (union AnyValue *)&EISCONNValue, FALSE);
#endif

#ifdef EISDIR
    VariableDefinePlatformVar(pc, NULL, "EISDIR", &pc->IntType, (union AnyValue *)&EISDIRValue, FALSE);
#endif

#ifdef ELOOP
    VariableDefinePlatformVar(pc, NULL, "ELOOP", &pc->IntType, (union AnyValue *)&ELOOPValue, FALSE);
#endif

#ifdef EMFILE
    VariableDefinePlatformVar(pc, NULL, "EMFILE", &pc->IntType, (union AnyValue *)&EMFILEValue, FALSE);
#endif

#ifdef EMLINK
    VariableDefinePlatformVar(pc, NULL, "EMLINK", &pc->IntType, (union AnyValue *)&EMLINKValue, FALSE);
#endif

#ifdef EMSGSIZE
    VariableDefinePlatformVar(pc, NULL, "EMSGSIZE", &pc->IntType, (union AnyValue *)&EMSGSIZEValue, FALSE);
#endif

#ifdef EMULTIHOP
    VariableDefinePlatformVar(pc, NULL, "EMULTIHOP", &pc->IntType, (union AnyValue *)&EMULTIHOPValue, FALSE);
#endif

#ifdef ENAMETOOLONG
    VariableDefinePlatformVar(pc, NULL, "ENAMETOOLONG", &pc->IntType, (union AnyValue *)&ENAMETOOLONGValue, FALSE);
#endif

#ifdef ENETDOWN
    VariableDefinePlatformVar(pc, NULL, "ENETDOWN", &pc->IntType, (union AnyValue *)&ENETDOWNValue, FALSE);
#endif

#ifdef ENETRESET
    VariableDefinePlatformVar(pc, NULL, "ENETRESET", &pc->IntType, (union AnyValue *)&ENETRESETValue, FALSE);
#endif

#ifdef ENETUNREACH
    VariableDefinePlatformVar(pc, NULL, "ENETUNREACH", &pc->IntType, (union AnyValue *)&ENETUNREACHValue, FALSE);
#endif

#ifdef ENFILE
    VariableDefinePlatformVar(pc, NULL, "ENFILE", &pc->IntType, (union AnyValue *)&ENFILEValue, FALSE);
#endif

#ifdef ENOBUFS
    VariableDefinePlatformVar(pc, NULL, "ENOBUFS", &pc->IntType, (union AnyValue *)&ENOBUFSValue, FALSE);
#endif

#ifdef ENODATA
    VariableDefinePlatformVar(pc, NULL, "ENODATA", &pc->IntType, (union AnyValue *)&ENODATAValue, FALSE);
#endif

#ifdef ENODEV
    VariableDefinePlatformVar(pc, NULL, "ENODEV", &pc->IntType, (union AnyValue *)&ENODEVValue, FALSE);
#endif

#ifdef ENOENT
    VariableDefinePlatformVar(pc, NULL, "ENOENT", &pc->IntType, (union AnyValue *)&ENOENTValue, FALSE);
#endif

#ifdef ENOEXEC
    VariableDefinePlatformVar(pc, NULL, "ENOEXEC", &pc->IntType, (union AnyValue *)&ENOEXECValue, FALSE);
#endif

#ifdef ENOLCK
    VariableDefinePlatformVar(pc, NULL, "ENOLCK", &pc->IntType, (union AnyValue *)&ENOLCKValue, FALSE);
#endif

#ifdef ENOLINK
    VariableDefinePlatformVar(pc, NULL, "ENOLINK", &pc->IntType, (union AnyValue *)&ENOLINKValue, FALSE);
#endif

#ifdef ENOMEM
    VariableDefinePlatformVar(pc, NULL, "ENOMEM", &pc->IntType, (union AnyValue *)&ENOMEMValue, FALSE);
#endif

#ifdef ENOMSG
    VariableDefinePlatformVar(pc, NULL, "ENOMSG", &pc->IntType, (union AnyValue *)&ENOMSGValue, FALSE);
#endif

#ifdef ENOPROTOOPT
    VariableDefinePlatformVar(pc, NULL, "ENOPROTOOPT", &pc->IntType, (union AnyValue *)&ENOPROTOOPTValue, FALSE);
#endif

#ifdef ENOSPC
    VariableDefinePlatformVar(pc, NULL, "ENOSPC", &pc->IntType, (union AnyValue *)&ENOSPCValue, FALSE);
#endif

#ifdef ENOSR
    VariableDefinePlatformVar(pc, NULL, "ENOSR", &pc->IntType, (union AnyValue *)&ENOSRValue, FALSE);
#endif

#ifdef ENOSTR
    VariableDefinePlatformVar(pc, NULL, "ENOSTR", &pc->IntType, (union AnyValue *)&ENOSTRValue, FALSE);
#endif

#ifdef ENOSYS
    VariableDefinePlatformVar(pc, NULL, "ENOSYS", &pc->IntType, (union AnyValue *)&ENOSYSValue, FALSE);
#endif

#ifdef ENOTCONN
    VariableDefinePlatformVar(pc, NULL, "ENOTCONN", &pc->IntType, (union AnyValue *)&ENOTCONNValue, FALSE);
#endif

#ifdef ENOTDIR
    VariableDefinePlatformVar(pc, NULL, "ENOTDIR", &pc->IntType, (union AnyValue *)&ENOTDIRValue, FALSE);
#endif

#ifdef ENOTEMPTY
    VariableDefinePlatformVar(pc, NULL, "ENOTEMPTY", &pc->IntType, (union AnyValue *)&ENOTEMPTYValue, FALSE);
#endif

#ifdef ENOTRECOVERABLE
    VariableDefinePlatformVar(pc, NULL, "ENOTRECOVERABLE", &pc->IntType, (union AnyValue *)&ENOTRECOVERABLEValue, FALSE);
#endif

#ifdef ENOTSOCK
    VariableDefinePlatformVar(pc, NULL, "ENOTSOCK", &pc->IntType, (union AnyValue *)&ENOTSOCKValue, FALSE);
#endif

#ifdef ENOTSUP
    VariableDefinePlatformVar(pc, NULL, "ENOTSUP", &pc->IntType, (union AnyValue *)&ENOTSUPValue, FALSE);
#endif

#ifdef ENOTTY
    VariableDefinePlatformVar(pc, NULL, "ENOTTY", &pc->IntType, (union AnyValue *)&ENOTTYValue, FALSE);
#endif

#ifdef ENXIO
    VariableDefinePlatformVar(pc, NULL, "ENXIO", &pc->IntType, (union AnyValue *)&ENXIOValue, FALSE);
#endif

#ifdef EOPNOTSUPP
    VariableDefinePlatformVar(pc, NULL, "EOPNOTSUPP", &pc->IntType, (union AnyValue *)&EOPNOTSUPPValue, FALSE);
#endif

#ifdef EOVERFLOW
    VariableDefinePlatformVar(pc, NULL, "EOVERFLOW", &pc->IntType, (union AnyValue *)&EOVERFLOWValue, FALSE);
#endif

#ifdef EOWNERDEAD
    VariableDefinePlatformVar(pc, NULL, "EOWNERDEAD", &pc->IntType, (union AnyValue *)&EOWNERDEADValue, FALSE);
#endif

#ifdef EPERM
    VariableDefinePlatformVar(pc, NULL, "EPERM", &pc->IntType, (union AnyValue *)&EPERMValue, FALSE);
#endif

#ifdef EPIPE
    VariableDefinePlatformVar(pc, NULL, "EPIPE", &pc->IntType, (union AnyValue *)&EPIPEValue, FALSE);
#endif

#ifdef EPROTO
    VariableDefinePlatformVar(pc, NULL, "EPROTO", &pc->IntType, (union AnyValue *)&EPROTOValue, FALSE);
#endif

#ifdef EPROTONOSUPPORT
    VariableDefinePlatformVar(pc, NULL, "EPROTONOSUPPORT", &pc->IntType, (union AnyValue *)&EPROTONOSUPPORTValue, FALSE);
#endif

#ifdef EPROTOTYPE
    VariableDefinePlatformVar(pc, NULL, "EPROTOTYPE", &pc->IntType, (union AnyValue *)&EPROTOTYPEValue, FALSE);
#endif

#ifdef ERANGE
    VariableDefinePlatformVar(pc, NULL, "ERANGE", &pc->IntType, (union AnyValue *)&ERANGEValue, FALSE);
#endif

#ifdef EROFS
    VariableDefinePlatformVar(pc, NULL, "EROFS", &pc->IntType, (union AnyValue *)&EROFSValue, FALSE);
#endif

#ifdef ESPIPE
    VariableDefinePlatformVar(pc, NULL, "ESPIPE", &pc->IntType, (union AnyValue *)&ESPIPEValue, FALSE);
#endif

#ifdef ESRCH
    VariableDefinePlatformVar(pc, NULL, "ESRCH", &pc->IntType, (union AnyValue *)&ESRCHValue, FALSE);
#endif

#ifdef ESTALE
    VariableDefinePlatformVar(pc, NULL, "ESTALE", &pc->IntType, (union AnyValue *)&ESTALEValue, FALSE);
#endif

#ifdef ETIME
    VariableDefinePlatformVar(pc, NULL, "ETIME", &pc->IntType, (union AnyValue *)&ETIMEValue, FALSE);
#endif

#ifdef ETIMEDOUT
    VariableDefinePlatformVar(pc, NULL, "ETIMEDOUT", &pc->IntType, (union AnyValue *)&ETIMEDOUTValue, FALSE);
#endif

#ifdef ETXTBSY
    VariableDefinePlatformVar(pc, NULL, "ETXTBSY", &pc->IntType, (union AnyValue *)&ETXTBSYValue, FALSE);
#endif

#ifdef EWOULDBLOCK
    VariableDefinePlatformVar(pc, NULL, "EWOULDBLOCK", &pc->IntType, (union AnyValue *)&EWOULDBLOCKValue, FALSE);
#endif

#ifdef EXDEV
    VariableDefinePlatformVar(pc, NULL, "EXDEV", &pc->IntType, (union AnyValue *)&EXDEVValue, FALSE);
#endif

    /* 注册 errno 全局变量，指向系统 errno */
    VariableDefinePlatformVar(pc, NULL, "errno", &pc->IntType, (union AnyValue *)&errno, TRUE);
}

#endif /* !BUILTIN_MINI_STDLIB */
