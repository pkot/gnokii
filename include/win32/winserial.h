typedef void (*sigcallback)(char);
typedef void (*keepalive)();

int OpenConnection(char *szPort, sigcallback fn, keepalive ka);
int CloseConnection();
int WriteCommBlock(LPSTR lpByte, DWORD dwBytesToWrite);

int ReadCommBlock(LPSTR lpszBlock, int nMaxLength);
void device_changespeed(int speed);
void device_setdtrrts(int dtr, int rts);
