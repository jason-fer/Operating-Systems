#ifndef __REQUEST_H__

void requestHandle(request_buffer *buffer);
void requestHandleFifo(int fd);
void prepRequest(request_buffer *buffer);

#endif