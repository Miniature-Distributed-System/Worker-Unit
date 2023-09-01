# Packet/Data Receiver

The Receiver module is responsible for receiving the packets sent out by the server. It copies the packets and validates these packets.
it determines what type of data was sent by the server to thr worker. it breaks down the packet and extracts information from the data feilds pf packets.
And then it hands over the packet the next appropriate process.

## Working

### Receiver Process

- The packet is recevived by the socket, the socket forwards the packet to the receiver process which gets scheduled as an non-preemtable process.
- The packet first goes through `validatePacketHead` method which validates the packet. It checks if it conforms to the pattern given else its ditched.
- The packet is checked if it is a `Instance` type or `User Data` type packet using the header information.
- If `validatePacketHead` returns status `P_VALID` then the packet header is checked if its a handshake from server if `SP_HANDSHAKE` is present in header.
- If its a handshake the worker is assigned the worker id which is present in the 'id' feild of every packet, this packet is not acknowledged.
- If packet header contains `SP_DATA_SENT` then it is either instance or user data depending on what `validatePacketHead` method found out we either pass it to `UserDataParser` or `InstanceDataParser`.
  This received data must be acknowledged by worker to server, therefore if its prased successfully the data is acked else its treated as invalid data and dropped.
- if packet header contains `SP_INTR_ACK` then it is a acknowledge packet sent by server acknowleding that it received the intermidiate data.
- if packet header contains `SP_FRES_ACK` then it is a acknowledge packet sent by server acknowleding that it received the final data.

### Receiver Finalize

- If the data is user data the sender unit is sent the table id, `DAT_RECVD` flag, and data priority to the `pushPacket` method.
- Else if its of type instance data the same parameters are sent with `DEFAULT_PRIORITY`.
- This data is passed to the data processor if we got user data
- Once `init_data_processor` completes the memory is cleaned up.

### Receiver Fail

- In case of failure the `Receiver` object is checked with all feilds to determine what type of error.
- If table id is missing then data is dietched.
- If table id is present and any other feild is incorrect the worker pushed to sender the table id with the error flag `RECV_ERR` wih priority set to highest.
- Once done the memory is cleaned up.
