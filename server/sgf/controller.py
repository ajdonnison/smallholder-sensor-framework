"""
    Controller class to handle the sensor network.
    This uses real-time updates as well as configuation options
    to build a flexible network view that then allows different
    components to be connected together as required without having
    to hard code destinations into the various sensors.

    It is envisaged that the ZigBee that is connected to this
    class is the network coordinator.  Future versions may not
    require this.
"""
import xbee
import serial
import time
import sys

class ZigBee(object):
    zigbee = None
    discovered = {}
    # Top level messages we are likely to get
    process_list = {
        'ID': 'handle_id_response',
        'ST': 'handle_status',
        'NK': 'handle_nak',
        'ER': 'handle_error',
        'CF': 'handle_config',
    }

    # top level handlers are based on the node ID
    top_level_handlers = {
        'node_id_indicator': 'read_node_id',
        'rx': 'read_data',
        'rx_io_data_log_addr': 'read_samples',
        'at_response': 'read_at',
        'tx_status': 'read_tx_status',
    }

    # Command handlers
    command_handlers = {
        'q': 'shutdown',
        's': 'request_status',
        'i': 'request_id',
        'c': 'request_config',
        'n': 'node_discover',
        'k': 'set_config',
    }

    def __init__(self, serial_obj):
        self.zigbee = xbee.ZigBee(serial_obj, callback=self.handler)

    def handle_response(self,source, args):
        if args[1] in self.response_list:
            method = getattr(self, self.response_list[args[1]])
            method(source, args)
        else:
            print "Unknown Response packet:", args

    def handle_nak(self, source, args):
        if source in self.discovered:
            node_id = self.discovered[source]['node_id']
        else:
            node_id = "UNKNOWN"

        print node_id, ": NAK received"

    def handle_status(self, source, args):
        if source in self.discovered:
            node_id = self.discovered[source]['node_id']
        else:
            node_id = "UNKNOWN"
        print node_id, ": Status:", args

    def handle_error(self, source, args):
        if source in self.discovered:
            node_id = self.discovered[source]['node_id']
        else:
            node_id = "UNKNOWN"
        print node_id, ": Error:", args

    def handle_id_response(self, source, args):
        if source in self.discovered:
            node_id = self.discovered[source]['node_id']
        else:
            node_id = "UNKNOWN"
        print node_id, ": ID:", args

    def handle_config(self, source, args):
        if source in self.discovered:
            node_id = self.discovered[source]['node_id']
        else:
            node_id = "UNKNOWN"
        print node_id, ": CF:", args

    def handle_on_response(self, source, args):
        if source in self.discovered:
            node_id = self.discovered[source]['node_id']
        else:
            node_id = "UNKNOWN"
        print node_id, ": ON:", args

    def handle_off_response(self, source, args):
        if source in self.discovered:
            node_id = self.discovered[source]['node_id']
        else:
            node_id = "UNKNOWN"
        print node_id, ": OFF", args

    # Cribbed from the XBee library
    def parse_nid(self, data):
        structure = [
            {'name':'source_addr',     'len':2},
            {'name':'source_addr_long','len':8},
            {'name':'node_id',         'len':'null_terminated'},
            {'name':'parent_source_addr','len':2},
            {'name':'device_type',     'len':1},
            {'name':'source_event',    'len':1},
            {'name':'digi_profile_id', 'len':2},
            {'name':'manufacturer_id', 'len':2}
        ]
        struct = {}

        index = 0
        for field in structure:
            if field['len'] == 'null_terminated':
                field_data = ''
                while data[index] != '\x00':
                    field_data += data[index]
                    index += 1
                index += 1
                struct[field['name']] = field_data
            elif field['len'] is not None:
                if index + field['len'] > len(data):
                    print field, len(data)
                    raise ValueError("packet too short")
                field_data = data[index:index + field['len']]
                struct[field['name']] = field_data
                index += field['len']
            else:
                field_data = data[index:]
                if field_Data:
                    struct[field['name']] = field_data
                    index += len(field_data)
                break

        return struct

    def handle_unknown(self, data):
        print data

    def read_node_id(self, data):
        print "Node ID: ", data['node_id'], " Source Addr:", [data['source_addr']]
        if data['source_addr'] in self.discovered:
            if self.discovered[data['source_addr']]['node_id'] != data['node_id']:
                self.discovered[data['source_addr']] = data
        else:
            self.discovered[data['source_addr']] = data
            # Send out a request for an ID
            self.send("tx", data="ID?", dest_addr = data['source_addr'], dest_addr_long = data['source_addr_long'])

    def read_data(self, data):
        args = data['rf_data'].split(':')
        source_addr = '\x00\x00'
        if 'source_addr' in data:
            source_addr = data['source_addr']
        if args[0] in self.process_list:
            method = getattr(self, self.process_list[args[0]])
            method(source_addr, args)
        else:
            print "Unknown packed type ", args[0]
            self.handle_unknown(data)

    def read_samples(self, data):
        # Do something useful
        pass

    # Don't do anything
    def noop(self, data):
        pass

    def read_at(self, data):
        if data['command'] == 'ND':
            data = self.parse_nid(data['parameter'])
            self.read_node_id(data)
        else:
            print "Unknown AT command"
            self.handle_unknown(data)

    def read_tx_status(self, data):
        if data['retries'] > '\x00' or data['deliver_status'] != '\x00':
            print "TX Status: Retries=", data['retries'], ", Deliver Status=", data['deliver_status']

    def handler(self, data):
        if data['id'] in self.top_level_handlers:
            method = getattr(self, self.top_level_handlers[data['id']])
            method(data)
        else:
            print "Unknown top level"
            self.handle_unknown(data)

    def send(self, cmd, **data):
        self.zigbee.send(cmd, **data)

    def send_at(self, data):
        self.zigbee.send('at', command=data)

    def stop(self):
        self.zigbee.halt()

    def shutdown(self, data):
        self.stop()
        raise Exception('Shutting Down')

    def request(self, cmd):
        for target in self.discovered:
            print  cmd, "> ", self.discovered[target]['node_id']
            self.send("tx", data=cmd, dest_addr = self.discovered[target]['source_addr'], dest_addr_long = self.discovered[target]['source_addr_long'])
            time.sleep(1)

    def request_from(self, node, cmd):
        for target in self.discovered:
            if node in self.discovered[target]['node_id']:
                print  cmd, "> ", self.discovered[target]['node_id']
                self.send("tx", data=cmd, dest_addr = self.discovered[target]['source_addr'], dest_addr_long = self.discovered[target]['source_addr_long'])

    def request_status(self, data):
        self.request("ST?")

    def request_config(self, data):
        self.request("CF?")

    def request_id(self, data):
        self.request("ID?")

    def node_discover(self, data):
        self.send_at('ND')

    def set_config(self, data):
        cmd_string = 'CF'
        node_id = data[1]
        for x in data[2:]:
          cmd_string = cmd_string + ':' + x
        self.request_from(node_id, cmd_string)

    def execute(self, cmd):
        if not cmd:
            return
        cmd_words = cmd.split()
        base_cmd = cmd_words[0].lower()
        if base_cmd in self.command_handlers:
            method = getattr(self, self.command_handlers[base_cmd])
            method(cmd_words)
        else:
            print "Unknown or unsupported command"

if __name__ == "__main__":
    ser = serial.Serial('/dev/ttyUSB0', 9600)
    bc = ZigBee(ser)
    time.sleep(1)
    bc.send_at('ND')
    while True:
        try:
            time.sleep(0.01)
        except KeyboardInterrupt:
            cmd = raw_input("Command: ")
            bc.execute(cmd)
        except:
            break
    bc.stop()
    ser.close()
    
# vim:ai sw=4 expandtab:
