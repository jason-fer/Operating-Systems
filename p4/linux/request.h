#ifndef __REQUEST_H__

void requestHandle(request_buffer *buffer);
void queueRequest(request_buffer *buffer);
void dequeueRequest(request_buffer *buffer);

#endif
