Store here your certs for mqtt broker

Load certificate file:
But you must convert it to .der
openssl x509 -in ./certs/IoLed_controller/client.crt -out ./certs/IoLed_controller/cert.der -outform DER

Load private key:
But you must convert it to .der
openssl rsa -in ./certs/IoLed_controller/client.key -out ./certs/IoLed_controller/private.der -outform DER
