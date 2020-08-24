/*
MIT License

Copyright(c) 2018 Liam Bindle

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

extern "C" {
   #include "mqtt_c.h"
}

/**
 * @file
 * @brief Implements @ref mqtt_pal_sendall and @ref mqtt_pal_recvall and
 *        any platform-specific helpers you'd like.
 * @cond Doxygen_Suppress
 */

#ifdef MQTT_USE_BIO

# include <openssl/bio.h>
# include <openssl/ssl.h>
# include <openssl/err.h>

ssize_t mqtt_pal_sendall(mqtt_pal_socket_handle fd, const void* buf, size_t len, int flags)
{
   size_t sent = 0;

   while (sent < len)
   {
      int tmp = BIO_write(fd, buf + sent, len - sent);

      if (tmp > 0)
         sent += (size_t) tmp;

      else if (tmp <= 0 && !BIO_should_retry(fd))
         return MQTT_ERROR_SOCKET_ERROR;
   }

   return sent;
}

ssize_t mqtt_pal_recvall(mqtt_pal_socket_handle fd, void* buf, size_t bufsz, int flags)
{
   const void const* start = buf;
   int rv;

   do
   {
      rv = BIO_read(fd, buf, bufsz);

      if (rv > 0)
      {
         // successfully read bytes from the socket

         buf += rv;
         bufsz -= rv;
      }

      else if (!BIO_should_retry(fd))
      {
         return MQTT_ERROR_SOCKET_ERROR;           // an error occurred
      }

   } while (!BIO_should_read(fd));

   return (ssize_t)(buf - start);
}

#else // MQTT_USE_BIO

#  include <errno.h>
#  include <stdio.h>
#  include <unistd.h>

ssize_t mqtt_pal_sendall(mqtt_pal_socket_handle fd, const char* buf, size_t len, int flags)
{
   size_t sent = 0;

   if (fd == -1)
      return MQTT_ERROR_SOCKET_ERROR;

   while (sent < len)
   {
      ssize_t tmp = send(fd, buf + sent, len - sent, flags);

      if (tmp < 1)
      {
         if (errno == EAGAIN || errno == EWOULDBLOCK)
         {
            usleep(100);
            continue;
         }

         if (errno == EINTR)
         {
            printf("Debug: send() interrupted by signal\n");
            // timeout occurred
         }

         printf("::send() result was (%zu) '%s'\n", tmp, strerror(errno));
         return MQTT_ERROR_SOCKET_ERROR;
      }

      sent += (size_t)tmp;
   }

   return sent;
}

ssize_t mqtt_pal_recvall(mqtt_pal_socket_handle fd, char* buf, size_t bufsz, int flags)
{
   const char* start = buf;
   ssize_t rv;

   if (fd == -1)
      return MQTT_ERROR_SOCKET_ERROR;

   do
   {
      rv = recv(fd, buf, bufsz, flags);

      if (rv > 0)
      {
         // successfully read bytes from the socket

         buf += rv;
         bufsz -= rv;
      }

      else if (errno == EINTR)
      {
         printf("Debug: recv() interrupted by signal\n");
         return 0;
      }

      else if (rv < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
      {
         printf("::recv() result was (%zu) '%s'\n", rv, strerror(errno));
         return MQTT_ERROR_SOCKET_ERROR;
      }

   }  while (rv > 0);

   return buf - start;
}

#endif // MQTT_USE_BIO
