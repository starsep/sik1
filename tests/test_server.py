from os import urandom
from random import randint, random, randrange
from signal import SIGINT
from socket import gethostname
# from string import ascii_lowercase
from subprocess import Popen, PIPE
from time import sleep

from MySocket import MySocket

MAX_CONNECTIONS = 20
MAX_EXPECTED_MESSAGES = 1  # > 1 ???
MIN_BYTES = 0
MAX_BYTES = 1000
ITERATIONS = 10 ** 4

hostname = gethostname()
port = randint(3210, 4321)
address = hostname, port

connections = []
expected_messages = []


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


def check_expected(index=None):
    if index is None:
        index = randrange(len(expected_messages))
    if expected_messages[index]:
        to_check = randint(1, len(expected_messages[index]))
        for expected_message in expected_messages[index][:to_check]:
            actual_message = connections[index].myreceive(len(expected_message))
            assert expected_message == actual_message
        expected_messages[index] = expected_messages[index][to_check:]


def check_max_expected_messages():
    for index in range(len(expected_messages)):
        while len(expected_messages[index]) >= MAX_EXPECTED_MESSAGES:
            check_expected(index)


def expect_except(message, index):
    for i in range(len(expected_messages)):
        if i != index:
            expected_messages[i].append(message)


def create_connection():
    print('create connection')
    socket = MySocket()
    socket.connect(hostname, port)
    connections.append(socket)
    expected_messages.append([])


def close_connection():
    print('close connection')
    index = randrange(len(connections))
    check_expected(index)
    connections[index].myclose()
    connections.pop(index)
    expected_messages.pop(index)


def send_message():
    index = randrange(len(connections))
    line = generate_random_bytes_line()
    print('send message (max_expected = %s, index = %s, length = %s)' % (
        max(map(len, expected_messages)), index, len(line)))
    # print('send message {line} from {index}'.format(**locals()))
    length_in_network_bytes = get_length_in_network_bytes(line)
    message = length_in_network_bytes + line
    connections[index].mysend(message)
    expect_except(message, index)
    # print(expected_messages)


server = Popen(['../server', str(port)], stdin=PIPE, stdout=PIPE)
sleep(0.01)  # żeby serwer zdążył się stworzyć
try:
    for iteration in range(ITERATIONS):
        sleep(0.02)  # ???
        if iteration % 1 == 0:
            print('\niteration %d (%d connections)' % (iteration, len(connections)))
            # print(expected_messages)
        action = random()
        if action < 0.1 and len(connections) < MAX_CONNECTIONS:
            create_connection()
        elif action < 0.45 and connections:
            check_max_expected_messages()
            send_message()
        elif action < 0.5 and connections:
            close_connection()
        elif connections:
            check_expected()
finally:
    server.send_signal(SIGINT)
    output, error = server.communicate()
    assert not output
    assert not error
    assert server.returncode == -SIGINT
