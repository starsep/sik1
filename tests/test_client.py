from os import urandom
from random import randint, random
from socket import AF_INET, gethostname, SOCK_STREAM, socket
from subprocess import Popen, PIPE

from MySocket import MySocket

ITERATIONS = 10 ** 4
MIN_BYTES = 0
MAX_BYTES = 1000
MAX_FROM_CLIENT = 1  # > 1 ???
MAX_FROM_SERVER = 53

serversocket = socket(AF_INET, SOCK_STREAM)

hostname = gethostname()
port = randint(3210, 4321)

serversocket.bind((hostname, port))
serversocket.listen(5)

print(hostname)

client = Popen(['../client', hostname, str(port)], stdin=PIPE, stdout=PIPE)

(clientsocket, _) = serversocket.accept()
clientsocket = MySocket(clientsocket)

from_client = []
from_server = []


def get_length_in_network_bytes(data):
    return len(data).to_bytes(2, byteorder='big')


'''
def generate_random_asci_lowercase_line():
    length = randint(MIN_BYTES, MAX_BYTES)
    chars = (choice(ascii_lowercase) for _ in range(length))
    line = ''.join(chars)
    return line.encode(encoding='UTF-8')
'''


def generate_random_bytes_line():
    length = randint(MIN_BYTES, MAX_BYTES)
    bytes = urandom(length).replace(b'\0', b'a').replace(b'\n', b'a')
    return bytes


for iteration in range(ITERATIONS):
    if iteration % 10 == 0:
        print('iteration %s' % iteration)

    action = random()

    if ((action < 0.05 and from_client)
        or (action < 0.5 and len(from_client) >= MAX_FROM_CLIENT)):
        number_to_check = randint(1, len(from_client))
        # print('recv from client (%s of %s)' %
        #      (number_to_check, len(from_client)))

        for b in from_client[:number_to_check]:
            # print('recv from client')
            actual_length = clientsocket.myreceive(2)
            expected_length = get_length_in_network_bytes(b)
            # print(expected_length)
            # print(actual_length)
            assert actual_length == expected_length
            actual_data = clientsocket.myreceive(len(b))
            # print(b)
            # print(actual_data)
            assert actual_data == b
        from_client = from_client[number_to_check:]
    elif action < 0.5:
        # print('write from client')
        b = generate_random_bytes_line()
        client.stdin.write(b + b'\n')
        client.stdin.flush()
        from_client.append(b)
    elif ((action < 0.55 and from_server)
          or len(from_server) >= MAX_FROM_SERVER):
        number_to_check = randint(1, len(from_server))
        # print('read from server (%s of %s)' %
        #      (number_to_check, len(from_server)))
        for b in from_server[:number_to_check]:
            actual = client.stdout.readline()
            assert actual == b + b'\n'
        from_server = from_server[number_to_check:]
    else:
        # print('send from server')
        b = generate_random_bytes_line()
        clientsocket.mysend(get_length_in_network_bytes(b) + b)
        from_server.append(b)

for b in from_server:
    actual = client.stdout.readline()
    assert actual == b + b'\n'
from_server = []

output, error = client.communicate()
expected_output = b''  # .join(line + b'\n' for line in from_server)
# print(expected_output)
# print(output)
assert output == expected_output
assert not error
assert client.returncode == 0

for b in from_client:
    # print('recv from client')
    actual_length = clientsocket.myreceive(2)
    expected_length = get_length_in_network_bytes(b)
    # print(expected_length)
    # print(actual_length)
    assert actual_length == expected_length
    actual_data = clientsocket.myreceive(len(b))
    # print(b)
    # print(actual_data)
    assert actual_data == b
from_client = []
