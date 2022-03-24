mkdir build
g++ server.cpp -o ./build/server -lpthread
g++ client.cpp -o ./build/client -lpthread