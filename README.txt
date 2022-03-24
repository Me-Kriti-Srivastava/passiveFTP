Run "sh build.sh"
Run server (from build directory): "./server <PORT>"
Run client (from build directory): "./client <ip> <port>"

Commands:
1. GET: 
    Usage: "GET <filename> [-b (for binary mode)] $"
    Example: "GET test.jpeg -b $"
    Description: Downloads a file from the logged-in user's directory from the 
                 server and saves it in the "CLIENT" directory on the client.

2. PUT: 
    Usage: "PUT <filename> [-b (for binary mode)] $"
    Example: "PUT test.jpeg -b $"
    Description: Uploads a file from the "CLIENT" directory from client to the 
                 logged-in user's directory on the server.

3. close: 
    Usage: "close $"
    Description: Disconnect the client from server.