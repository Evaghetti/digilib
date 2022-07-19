from __future__ import annotations
from queue import Queue
import socket
from typing import Dict, List, Tuple, Union
import select
from struct import *
from urllib import response
from uuid import uuid4 as uuid

# TODO: Leave match results on server side instead of client side

# Actions
BATTLE_REGISTER = 0
BATTLE_CHALLENGE = 1
BATTLE_REQUEST = 2
BATTLE_LIST = 3
UPDATE_GAME = 4

# Tags TLV
SLOT_POWER = 0 # 1 bytes length
VERSION = 1    # 1 byte length
COUNT = 2      # 4 byte length
USER_ID = 3    # 36 byte length
RESPONSE = 4   # 1 byte length

# Interaction request status
NO_RESPONSE = 0
ACCEPTED = 1
REFUSED = 2

# Constants
ADDRESS = 'localhost'
PORT = 1998
MAX_PACKET_LEN = 1024

def getLengthFormat(length: int) -> str:
    if length == 1:
        return f"<b"
    elif length == 2:
        return f"<h"
    else:
        return f"<{length}s"

class Player:
    def __init__(self) -> None:
        self.uuid = str(uuid())
        self.currentAction = BATTLE_REGISTER
        self.slotPower = 0
        self.version = 0
        self.challenging: Player = None
        self.challengedBy: Player = None
        self.requestStatus: int = NO_RESPONSE

        print("New user", self.uuid)

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

    def isInteracting(self, another: Player = None) -> bool:
        if another is None:
            return self.challenging != None or self.challengedBy != None

        return another is self.challenging or another is self.challengedBy

    def getInteractingPlayer(self) -> Player:
        return self.challenging or self.challengedBy
    
    def isBattling(self) -> bool:
        if not self.isInteracting():
            return False

        return self.requestStatus == ACCEPTED

    def cleanup(self):
        if self.challenging:
            self.challenging.requestStatus = REFUSED
            self.challenging.challengedBy = None
        if self.challengedBy:
            self.challengedBy.requestStatus = REFUSED
            self.challengedBy = None

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

                    if user.isBattling():
                        print(f"Battle logic from {user.uuid}")
                        otherUser: Player = user.getInteractingPlayer()
                        socketOtherUser, otherUser = self.lookForUser(otherUser.uuid)

                        print(f"Sending to {otherUser.uuid}")
                        self.send(socketOtherUser, dataReceive)
                        continue

                    user.currentAction = dataReceive[0]
                    receivedTLV = self.processTLV(dataReceive[1:]) if len(dataReceive) > 1 else {}

                    print(f"Received data {user.uuid}:", ''.join('\\x{:02x}'.format(b) for b in dataReceive))
                        
                    if user.currentAction == BATTLE_REGISTER:
                        user.configure(dataReceive[1:])
                        self.send(input, 0)
                    elif user.currentAction == BATTLE_CHALLENGE:
                        uuidChallenged: str = receivedTLV[USER_ID].decode()

                        print(f"{user.uuid} is trying to challenge {uuidChallenged}")
                        socketChallenged, userChallenged = self.lookForUser(uuidChallenged)

                        if not userChallenged.isInteracting():
                            userChallenged.challengedBy = user
                            user.challenging = userChallenged

                            print(f"{user.uuid} challenged {uuidChallenged}")
                            self.send(input, 1)
                        else:
                            print(f"{user.uuid} was not able to challenge {uuidChallenged}, he was interacting with another")
                            self.send(input, 0)
                            
                    elif user.currentAction == UPDATE_GAME:
                        if user.isInteracting():
                            print(f"Processing user interaction")
                            if user.challenging:
                                print("Challenger logic")

                                # The status of this request is the same as the challenged
                                user.requestStatus = user.challenging.requestStatus 

                                self.send(input, BATTLE_CHALLENGE)
                                self.send(input, pack("<bbb", RESPONSE, 1, user.requestStatus))

                                if user.requestStatus == REFUSED:
                                    print(f"{user.challenging.uuid} refused your challenge {user.uuid}")
                                    user.challenging = None
                                elif user.requestStatus == ACCEPTED:
                                    print(f"{user.challenging.uuid} accepted your challenge {user.uuid}")
                            else:
                                print("Challenged logic")
                                self.send(input, BATTLE_REQUEST)

                                if user.requestStatus == NO_RESPONSE:
                                    user.requestStatus = receivedTLV[RESPONSE]
                                self.send(input, pack("<bbb", RESPONSE, 1, user.requestStatus))
                                self.send(input, pack("<bb", USER_ID, len(user.challengedBy.uuid)) + user.challengedBy.uuid.encode())

                                if user.requestStatus == ACCEPTED:
                                    print(f"{user.uuid} accepted challenge by {user.challengedBy.uuid}")
                                elif user.requestStatus == REFUSED:
                                    print(f"{user.uuid} refused challenge by {user.challengedBy.uuid}")
                                    user.challengedBy = None
                                else:
                                    print(f"{user.uuid} still did not respond")
                        else:
                            self.send(input, BATTLE_LIST)
                            self.send(input, self.createPlayerList(user))
                else:
                    raise ConnectionResetError
            except ConnectionResetError:
                print("Removing")
                if input in self.outputs:
                    self.outputs.remove(input)
                self.inputs.remove(input)
                del self.messageQueue[input]
                self.users[input].cleanup()
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
    
    def send(self, target: socket.socket, data: Union[bytes, int]) -> None:
        if type(data) is int:
            data = bytes([data])
        dataStr: str = ''.join('\\x{:02x}'.format(b) for b in data)
        print(f"Sending data -> {dataStr}")

        self.messageQueue[target].put(data)
    
    def processTLV(self, data: bytes) -> Dict[int, bytes]:
        ret: dict[int, bytes] = {}

        while len(data) >= 3:
            tag, lengthRaw = unpack("<bb", data[0:2])
            length = getLengthFormat(lengthRaw)
            value = unpack(length, data[2:2 + lengthRaw])[0]

            ret[tag] = value
            data = data[2+lengthRaw:]
        return ret
        
    def lookForUser(self, idUser: str) -> Tuple[socket.socket, Player]:
        for socket in self.users.keys():
            if self.users[socket].uuid == idUser:
                return (socket, self.users[socket])
        else:
            print(f"No user {idUser}")
            raise

def main():
    server = Server()
    server.run()
    pass

if __name__ == "__main__":
    main()