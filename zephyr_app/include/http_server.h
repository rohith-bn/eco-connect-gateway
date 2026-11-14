#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <stdint.h>

/**
 * @brief Update the latest temperature (in hundredths of deg C) displayed on
 *        the static EcoConnect status page.
 */
void http_server_set_temp_x100(int32_t temp_c_x100);

/**
 * @brief Start the lightweight TCP/HTTP static web server on port 80.
 */
void http_server_start(void);

#endif /* HTTP_SERVER_H */
