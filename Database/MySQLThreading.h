#pragma once


namespace DATABASE
{
//多线程需要调用
void MysqlLibraryInit();
void MysqlLibrary_End();
unsigned int MysqlGetLibraryVersion();


}// namespace DATABASE