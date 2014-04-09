#ifndef __REQUEST_H__

void requestHandle(int fd);
void queueRequest(request_buffer *buffer, int fd);
void dequeueRequest(request_buffer *buffer);

#endif
