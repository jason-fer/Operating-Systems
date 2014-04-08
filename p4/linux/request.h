#ifndef __REQUEST_H__

void requestHandle(int fd);
void requestFillBuffer(request_buffer *buffer, int fd);

#endif
