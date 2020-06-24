const char url[] = {"/10.10.0.1/"};
const char hoststr[] = {"10.10.0.1"}; //const char website[] PROGMEM = "api-conference-rooms.int-24.com";
const char http_header[] 	= {"HTTP/1.1 200 OK\r\nServer: stm32confroom\r\nContent-Type: text/html\r\nConnection: keep-alive\r\n\r\n"};
const char error_header[] 	= {"HTTP/1.1 404 File not found\r\nServer: stm32confroom\r\nContent-Type: text/html\r\nConnection: keep-alive\r\n\r\n"};
const char lock_on_html[] = {"HTTP/1.1 201 OK\r\nServer: stm32confroom\r\nContent-Type: text/html\r\nConnection: keep-alive\r\n\r\n"
							"<head><h1>LOCK ON</br></br></head>"};
const char lock_off_html[] = {"HTTP/1.1 202 OK\r\nServer: stm32confroom\r\nContent-Type: text/html\r\nConnection: keep-alive\r\n\r\n"
							"<head><h1>LOCK OFF</br></br></head>"};
const char dhcp_page_true[] = {"%s<p><h3><input name='use_dhcp' type='radio' value='0'> use STATIC</h3></p>"
        					   "<p><h3><input name='use_dhcp' type='radio' value='1' checked> use DHCP</h3></p>"
								"<p><input type='submit' value='SAVE'></p>"
								"</form>"
								"</html>"
								"\n\r\n\r"};
const char dhcp_page_false[] = {"%s<p><h3><input name='use_dhcp' type='radio' value='0' checked> use STATIC</h3></p>"
                				"<p><h3><input name='use_dhcp' type='radio' value='1'> use DHCP</h3></p>"
								"<p><input type='submit' value='SAVE'></p>"
								"</body>"
								"</form>"
                				"</html>"
                				"\n\r\n\r"};
const char home_page[] = {"HTTP/1.1 200 OK\r\nServer: nginx\r\nContent-Type: text/html\r\nConnection: keep-alive\r\n\r\n"
						 //"<form> name='test' method='POST'"
						"<form>"
						"<head><h1>ConferenceRoom v2.0 build:140419</br><h2>(c) MINT innovations, 2019</br></br></head>"
						"<body>"
						"<h3>IP:</h3>"
						"<p><input type='text' name='ip0' id='ip1' value='%03u' size=2"
						"<p><input type='text' name='ip1' id='ip2' value='%03u' size=2"
						"<p><input type='text' name='ip2' id='ip3' value='%03u' size=2"
						"<p><input type='text' name='ip3' id='ip4' value='%03u' size=2</p>"
						"<h3>MAC:</h3>"
						"<p><input type='text' name='mac0' id='mac1' value='%02X' size=2"
						"<p><input type='text' name='mac1' id='mac2' value='%02X' size=2"
						"<p><input type='text' name='mac2' id='mac3' value='%02X' size=2"
						"<p><input type='text' name='mac3' id='mac4' value='%02X' size=2"
						"<p><input type='text' name='mac4' id='mac5' value='%02X' size=2"
						"<p><input type='text' name='mac5' id='mac6' value='%02X' size=2</p>"
						"<h3>DEST IP:</h3>"
						"<p><input type='text' name='dip0' id='dip1' value='%03u' size=2"
						"<p><input type='text' name='dip1' id='dip2' value='%03u' size=2"
						"<p><input type='text' name='dip2' id='dip3' value='%03u' size=2"
						"<p><input type='text' name='dip3' id='dip4' value='%03u' size=2</p>"
						"<h3>lock_sens[]={%01u,%01u},lock_status_hw[]={%01u,%01u}</h3>"};
