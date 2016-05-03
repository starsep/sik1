from os import urandom
from random import choice, randint, random, randrange
from signal import SIGINT
from string import ascii_lowercase
from subprocess import Popen, PIPE
from time import sleep

MAX_CLIENTS = 20
MIN_BYTES = 0
MAX_BYTES = 1000
ITERATIONS = 10 ** 4
CHECK_FREQUENCY = 1  # > 1 ???

port = str(randint(3210, 4321))

clients = []
expected_lines = []


def create_client():
    new_client = Popen(['../client', 'localhost', port], stdin=PIPE, stdout=PIPE)
    clients.append(new_client)
    expected_lines.append([])


def remove_client(index=-1):
    removed_client = clients.pop(index)
    expected_output = b''.join(expected_lines.pop(index))
    output, errors = removed_client.communicate()
    # print('\nremove_client:\n%s\n%s' % (expected_output, output))
    assert removed_client.returncode == 0
    assert output == expected_output
    assert not errors


def send_line(index, line):
    stdin = clients[index].stdin
    stdin.writelines([line])
    stdin.flush()


def generate_random_asci_lowercase_line():
    length = randint(MIN_BYTES, MAX_BYTES)
    chars = (choice(ascii_lowercase) for _ in range(length))
    line = ''.join(chars) + '\n'
    return line.encode(encoding='UTF-8')


def generate_random_bytes_line():
    length = randint(MIN_BYTES, MAX_BYTES)
    bytes = urandom(length).replace(b'\0', b'a').replace(b'\n', b'a')
    return bytes + b'\n'


def expect_except(line, index):
    for i in range(len(expected_lines)):
        if i != index:
            expected_lines[i].append(line)
'''
def check_except(line, index):
    for i, client in enumerate(clients):
        if i != index:
            client.stdin.flush()
            client.stdout.flush()
            print('\ncheck_except:\n%s' % line)
            output = client.stdout.readline()
            print(output)
            assert output == line
'''


def check_expected():
    for i, client in enumerate(clients):
        for expected_line in expected_lines[i]:
            # print('\ncheck_expected:\n%s' % expected_line)
            actual_line = client.stdout.readline()
            # print(actual_line)
            assert expected_line == actual_line
        expected_lines[i] = []


server = Popen(['../server', port], stdin=PIPE, stdout=PIPE)
try:
    for iteration in range(ITERATIONS):
        if iteration % 10 == 0:
            print('\niteration {}'.format(iteration))
        sleep(0.01)  # żeby kolejność wiadomości się nie mieszała
        if iteration % CHECK_FREQUENCY == 0:
            check_expected()
        action = random()
        if action < 0.2 and len(clients) < MAX_CLIENTS:
            # print('creating client %s' % len(clients))
            create_client()
        elif action < 0.9 and clients:
            index = randrange(len(clients))
            line = generate_random_bytes_line()
            #print('client %s sends %s' % (index, line))
            send_line(index, line)
            expect_except(line, index)
        elif clients:
            index = randrange(len(clients))
            #print('ending client %s' % index)
            remove_client(index)

    sleep(0.05)  # żeby zdążyć odebrać wszystkie wiadomości
    while clients:
        remove_client()
finally:
    server.send_signal(SIGINT)
    output, errors = server.communicate()
    assert server.returncode == -SIGINT
    assert output == b''
    assert not errors
