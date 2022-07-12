from queue import Queue
import socket
from typing import Dict, List
import select
from struct import *
from uuid import uuid4 as uuid

# TODO: Leave match results on server side instead of client side

# Actions
BATTLE_REGISTER = 0
BATTLE_CHALLENGE = 1
BATTLE_REQUEST = 2
BATTLE_LIST = 3

# Tags TLV
SLOT_POWER = 0 # 1 bytes length
VERSION = 1    # 1 byte length
COUNT = 2      # 4 byte length
USER_ID = 3    # 36 byte length

# Constants
ADDRESS = 'localhost'
PORT = 1998
MAX_PACKET_LEN = 1024

def getLengthFormat(length: int) -> str:
    if length == 1:
        return f"<b"
    elif length == 2:
        return f"<h"
    raise f"Invalid length of {length}"

class Player:
    def __init__(self) -> None:
        self.uuid = str(uuid())
        self.currentAction = BATTLE_REGISTER
        self.slotPower = 0
        self.version = 0

    def configure(self, data: bytes) -> None:
        if len(data) < 3:
            raise "Not enough data to configure player"
        
        while len(data) >= 3:
            tag = data[0]
            length = data[1]
            value = unpack(getLengthFormat(length), data[2:2 + length])[0]

            if tag == SLOT_POWER:
                self.slotPower = value
            elif tag == VERSION:
                self.version = value
            data = data[2+length:]

    def serialize(self) -> bytes:
        data = pack("<bbb", SLOT_POWER, 1, self.slotPower)
        data += pack("<bbb", VERSION, 1, self.version)
        data += pack("<bb", USER_ID, 36)
        data += self.uuid.encode()
        return data

class Server:
    def __init__(self) -> None:
        self.server: socket.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server.setblocking(0)
        self.server.bind((ADDRESS, PORT))
        self.server.listen(5)
        
        self.inputs: List[socket.socket] = [self.server]
        self.outputs: List[socket.socket] = []
        self.users: Dict[socket.socket, Player] = {}
        self.messageQueue: Dict[socket.socket, Queue] = {}

    def createPlayerList(self, ignore: Player) -> bytes:
        retData = pack("<bbi", COUNT, 2, len(self.users) - 1)
        for player in self.users.values():
            if player is ignore:
                continue

            retData += player.serialize()
        return bytes(retData)


    def run(self):
        while self.inputs:
            readable, writable, ignore = select.select(self.inputs, self.outputs, self.inputs)

            self.proccessInput(readable)
            self.proccessOutput(writable)

    def proccessInput(self, readable: List[socket.socket]) -> None:
        for input in readable:
            if input is self.server:
                newSocket, address = input.accept()
                print(f"Nova conexão de {address}")
                newSocket.setblocking(0)
                self.inputs.append(newSocket)
                self.messageQueue[newSocket] = Queue()
                self.users[newSocket] = Player()
                continue

            try:
                dataReceive: bytes = input.recv(MAX_PACKET_LEN)
                if dataReceive:
                    if input not in self.outputs:
                        self.outputs.append(input)

                    user: Player = self.users[input]
                    user.currentAction = dataReceive[0]

                    print("Received data:", ''.join('\\x{:02x}'.format(b) for b in dataReceive))
                    
                    if user.currentAction == BATTLE_REGISTER:
                        user.configure(dataReceive[1:])
                        data = self.createPlayerList(user)
                        print("Sending data:", ''.join('\\x{:02x}'.format(b) for b in data))
                        self.messageQueue[input].put(data)
                    elif user.currentAction == BATTLE_LIST:
                        self.messageQueue[input].put(self.createPlayerList(user))
                else:
                    raise ConnectionResetError
            except ConnectionResetError:
                print("Removing")
                if input in self.outputs:
                    self.outputs.remove(input)
                self.inputs.remove(input)
                del self.messageQueue[input]
                del self.users[input]

                input.close()

    def proccessOutput(self, writable: List[socket.socket]) -> None:
        for write in writable:
            try:
                message = self.messageQueue[write].get_nowait()
            except:
                pass
            else:
                write.send(message)

def main():
    server = Server()
    server.run()
    pass

if __name__ == "__main__":
    main()