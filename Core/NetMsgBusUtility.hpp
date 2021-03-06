#ifndef NETMSGBUS_UTILITY_H
#define NETMSGBUS_UTILITY_H

#include "msgbus_interface.h"
#include "TcpSock.h"
#include "SockWaiterBase.h"
#include "NetMsgBusFilterMgr.h"
#include "CommonUtility.hpp"
#include "SimpleLogger.h"
#include <string>
#include <stdio.h>
#include <boost/shared_array.hpp>
#include <fcntl.h>
#include <arpa/inet.h>

#define NETMSGBUS_EVLOOP_NAME "evloop_for_netmsgbus"
#define SENDER_LEN_BYTES 1
#define MSGKEY_LEN_BYTES 1
#define MSGVALUE_LEN_BYTES 4

using namespace core::net;

namespace NetMsgBus
{

static core::LoggerCategory g_log("NetMsgBusUtility");

static const std::string endcoded_percent_str = "%25";
static const std::string percent_str = "%";
static const std::string encoded_and_str = "%26";
static const std::string and_str = "&";
static const std::string equal_str = "=";
static const std::string msgid_str = "msgid";
static const std::string msgparam_str = "msgparam";
static const std::string msgsender_str = "msgsender";
inline void EncodeMsgKeyValue(std::string& orig_value)
{
    // no need encode using the fixed schema.
    return;
    // replace % and &
    size_t i = 0; 
    while(i < orig_value.size())
    {
        if(orig_value[i] == '%')
        {
            orig_value.replace(i, 1, endcoded_percent_str);
            i += 3;
        }
        else if(orig_value[i] == '&')
        {
            orig_value.replace(i, 1, encoded_and_str);
            i += 3;
        }
        else
        {
            ++i;
        }
    }
}

inline void DecodeMsgKeyValue(std::string& orig_value)
{
    return;
    // replace % 
    size_t i = 0; 
    while(i < orig_value.size())
    {
        if(orig_value[i] == '%')
        {
            if(orig_value[i+1]=='2' && orig_value[i+2] == '5')
                orig_value.replace(i, 3, percent_str);
            else if(orig_value[i+1]=='2' && orig_value[i+2] == '6')
                orig_value.replace(i, 3, and_str);
            else
            {
                printf("warning: unknow encode msgbus value string : %s.", orig_value.substr(i, 3).c_str());
                orig_value.replace(i, 3, "");
            }
        }
        ++i;
    }
}

inline bool GetMsgSender(const std::string& netmsgbus_msgcontent, std::string& sender)
{
    if (netmsgbus_msgcontent.length() < SENDER_LEN_BYTES)
      return false;
    uint8_t msgsender_len = (uint8_t)netmsgbus_msgcontent[0];
    if (netmsgbus_msgcontent.length() < SENDER_LEN_BYTES + (std::size_t)msgsender_len)
      return false;

    sender = netmsgbus_msgcontent.substr(SENDER_LEN_BYTES, msgsender_len);
    return true;
}

inline bool GetMsgId(const std::string& netmsgbus_msgcontent, std::string& msgid)
{
    if (netmsgbus_msgcontent.length() < SENDER_LEN_BYTES)
      return false;
    uint8_t msgsender_len = (uint8_t)netmsgbus_msgcontent[0];
    if (netmsgbus_msgcontent.length() < SENDER_LEN_BYTES + (std::size_t)msgsender_len + MSGKEY_LEN_BYTES)
      return false;
    uint8_t msgid_len = (uint8_t)(netmsgbus_msgcontent[SENDER_LEN_BYTES + msgsender_len]);
    if (netmsgbus_msgcontent.length() < SENDER_LEN_BYTES + (std::size_t)msgsender_len + MSGKEY_LEN_BYTES + (std::size_t)msgid_len)
      return false;
    msgid = netmsgbus_msgcontent.substr(SENDER_LEN_BYTES + (std::size_t)msgsender_len + MSGKEY_LEN_BYTES, msgid_len);
    return true;

    //std::size_t startpos = netmsgbus_msgcontent.find(msgkey + equal_str);
    //if(startpos != std::string::npos)
    //{
    //    std::size_t endpos = netmsgbus_msgcontent.find(and_str, startpos);
    //    if(endpos != std::string::npos)
    //    {
    //        msgvalue = std::string(netmsgbus_msgcontent, startpos + msgkey.size() + 1,
    //            endpos - startpos - (msgkey.size() + 1));
    //    }
    //    else
    //    {
    //        msgvalue = std::string(netmsgbus_msgcontent, startpos + msgkey.size() + 1);
    //    }
    //    DecodeMsgKeyValue(msgvalue);
    //    return true;
    //}
    // //printf("netmsgbus_msgcontent error, no %s found.\n", msgkey.c_str());
    //return false;
}

inline bool CheckMsgSender(const std::string& netmsgbus_msgcontent, std::string& msgsender)
{
    if(GetMsgSender(netmsgbus_msgcontent, msgsender))
    {
        return FilterMgr::FilterBySender(msgsender);
    }
    return false;
}

// 1 byte msg_sender len + length of msg sender + 1byte msgid length + length of msgid + 4bytes msgparam len + length of msgparam.
inline bool CheckMsgId(const std::string& netmsgbus_msgcontent, std::string& msgid)
{
    // 
    if(GetMsgId(netmsgbus_msgcontent, msgid))
    {
        return FilterMgr::FilterByMsgId(msgid);
    }
    return false;
}

inline bool GetMsgParam(const std::string& netmsgbus_msgcontent, boost::shared_array<char>& msgparam, uint32_t& param_len)
{
    std::string msgparamstr;
    if (netmsgbus_msgcontent.length() < SENDER_LEN_BYTES)
      return false;
    uint8_t msgsender_len = (uint8_t)netmsgbus_msgcontent[0];
    if (netmsgbus_msgcontent.length() < SENDER_LEN_BYTES + (std::size_t)msgsender_len + MSGKEY_LEN_BYTES)
      return false;
    uint8_t msgkey_len = (uint8_t)(netmsgbus_msgcontent[SENDER_LEN_BYTES + msgsender_len]);
    uint32_t msgparam_offset = SENDER_LEN_BYTES + msgsender_len + MSGKEY_LEN_BYTES + msgkey_len;
    if (netmsgbus_msgcontent.length() <  msgparam_offset + MSGVALUE_LEN_BYTES)
      return false;

    uint32_t netparam_len = *((uint32_t *)&netmsgbus_msgcontent[msgparam_offset]);
    param_len = ntohl(netparam_len);
    if (netmsgbus_msgcontent.length() < msgparam_offset + MSGVALUE_LEN_BYTES + param_len)
      return false;

    msgparam.reset(new char[param_len]);
    memcpy(msgparam.get(), &netmsgbus_msgcontent[msgparam_offset + MSGVALUE_LEN_BYTES], param_len);
    return true;

    //if(GetMsgKey(netmsgbus_msgcontent, msgparam_str, msgparamstr))
    //{
    //    assert(msgparamstr.size());
    //    msgparam.reset(new char[msgparamstr.size()]);
    //    memcpy(msgparam.get(), msgparamstr.data(), msgparamstr.size());
    //    param_len = msgparamstr.size();
    //    return true;
    //}
    //printf("netmsgbus_msgcontent error, no msgparam found.\n");
    //return false;
}

inline void GenerateNetMsgContent(const std::string& msgid, MsgBusParam param, const std::string& msgsender,
    MsgBusParam& netmsg_data)
{
    uint8_t msgsender_len = (uint8_t)msgsender.size();
    uint8_t msgid_len = (uint8_t)msgid.size();
    uint32_t netmsgparam_len = htonl(param.paramlen);
    netmsg_data.paramlen = sizeof(msgsender_len) + msgsender_len + sizeof(msgid_len) + msgid_len + 
      sizeof(netmsgparam_len) + param.paramlen;
    netmsg_data.paramdata.reset(new char[netmsg_data.paramlen]);
    char *pdata = netmsg_data.paramdata.get();
    memcpy(pdata, &msgsender_len, sizeof(msgsender_len));
    pdata += sizeof(msgsender_len);
    memcpy(pdata, (const char*)&msgsender[0], msgsender_len);
    pdata += msgsender_len;
    memcpy(pdata, (char*)&msgid_len, sizeof(msgid_len));
    pdata += sizeof(msgid_len);
    memcpy(pdata, (const char*)&msgid[0], msgid_len);
    pdata += msgid_len;
    memcpy(pdata, (char*)&netmsgparam_len, sizeof(netmsgparam_len));
    pdata += sizeof(netmsgparam_len);
    memcpy(pdata, param.paramdata.get(), param.paramlen);
    return;


    //const static std::string msgidstr = "msgid=";
    //const static std::string andmsgparamstr = "&msgparam=";
    //const static std::string andmsgsenderstr = "&msgsender=";
    //std::string netmsg_str(param.paramdata.get(), param.paramlen);
    //std::string encodemsgid = msgid;
    //std::string encode_msgsender = msgsender;
    //EncodeMsgKeyValue(encodemsgid);
    //EncodeMsgKeyValue(netmsg_str);
    //EncodeMsgKeyValue(encode_msgsender);
    //netmsg_str = msgidstr + encodemsgid + andmsgparamstr + netmsg_str + andmsgsenderstr + encode_msgsender;
    //netmsg_data.paramlen = netmsg_str.size();
    //netmsg_data.paramdata.reset(new char[netmsg_data.paramlen]);
    //memcpy(netmsg_data.paramdata.get(), netmsg_str.data(), netmsg_data.paramlen);
}

// response to the client who has send a msg using sync mode.
inline void NetMsgBusRspSendMsg(TcpSockSmartPtr sp_tcp, const std::string& netmsgbus_msgcontent, uint32_t sync_sid)
{
    //LOG(g_log, core::lv_debug, "process a sync request begin :%lld, sid:%u, fd:%d\n", (int64_t)core::utility::GetTickCount(), sync_sid, sp_tcp->GetFD());
    std::string msgid;
    if(CheckMsgId(netmsgbus_msgcontent, msgid))
    {
        boost::shared_array<char> data;
        uint32_t data_len;
        if(GetMsgParam(netmsgbus_msgcontent, data, data_len))
        {
            if(!SendMsg(msgid, data, data_len))
                data_len = 0;
            // when finished process, write the data back to the client.
            uint32_t sync_sid_n = htonl(sync_sid);
            uint32_t data_len_n = htonl(data_len);
            uint32_t write_len = sizeof(sync_sid_n) + sizeof(data_len_n) + data_len;
            char* writedata = new char[write_len];
            memcpy(writedata, &sync_sid_n, sizeof(sync_sid_n));
            memcpy(writedata + sizeof(sync_sid_n), (char*)&data_len_n, sizeof(data_len_n));
            memcpy(writedata + sizeof(sync_sid_n) + sizeof(data_len_n), data.get(), data_len);
            sp_tcp->SendData(writedata, write_len);
            //LOG(g_log, core::lv_debug, "process a sync request finished :%lld, sid:%u, fd:%d\n", (int64_t)core::utility::GetTickCount(), sync_sid, sp_tcp->GetFD());
        }
    }
}

inline void NetMsgBusToLocalMsgBus(const std::string& netmsgbus_msgcontent)
{
    std::string msgid;
    if(CheckMsgId(netmsgbus_msgcontent, msgid))
    {
        boost::shared_array<char> msgparam;
        uint32_t param_len;
        if(GetMsgParam(netmsgbus_msgcontent, msgparam, param_len))
        {
            PostMsg(msgid, msgparam, param_len);
        }
    }
}

}
#endif // end of NETMSGBUS_UTILITY_H
