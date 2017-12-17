iptables -t nat -A POSTROUTING -j SNAT --to-source 9.9.9.9 --random

create CA:
	openssl req -new -newkey rsa -keyform PEM -keyout cakey.pem -x509 -days 365 -set_serial 0x0 -out cacert.pem

create req:
	openssl req -newkey rsa -keyout user1_key.pem -out user1_req.pem
	(for server)openssl req -newkey rsa -extensions server -keyout server_key.pem -out server_req.pem

sign req:
	openssl ca -cert cacert.pem -keyfile cakey.pem -create_serial -in user1_req.pem -notext -outdir new_certs -out /dev/null
	(for server)openssl ca -cert cacert.pem -keyfile cakey.pem -create_serial -extensions server -in server_req.pem -notext -outdir new_certs -out /dev/null

view cert:
	openssl x509 -in XXX.pem -text -noout

revoke cert:
	openssl ca -cert cacert.pem -keyfile cakey.pem -revoke XXX.pem

update crl:
	openssl ca -cert cacert.pem -keyfile cakey.pem -gencrl -out crl.pem

generate dh:
	openssl dhparam -out dh2048.pem 2048
