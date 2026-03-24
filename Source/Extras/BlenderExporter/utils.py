import struct
import array
import re
from xml.etree import ElementTree as ET
from xml.dom import minidom


class BinaryFileWriter:
    def __init__(self):
        self.filename = None
        self.buffer = None

    def open(self, filename):
        self.filename = filename
        self.buffer = array.array('B')

    def close(self):
        with open(self.filename, "wb", 1024 * 1024) as f:
            self.buffer.tofile(f)

    def writeAsciiStr(self, v):
        v = re.sub(r'[^\x00-\x7f]', '_', v)
        self.buffer.extend(bytes(v, "ascii", errors="ignore"))

    def writeUInt(self, v):
        self.buffer.extend(struct.pack("<I", v))

    def writeUShort(self, v):
        self.buffer.extend(struct.pack("<H", v))

    def writeUByte(self, v):
        self.buffer.extend(struct.pack("<B", v))

    def writeFloat(self, v):
        self.buffer.extend(struct.pack("<f", v))

    def writeVector3(self, v):
        self.buffer.extend(struct.pack("<3f", v[0], v[1], v[2]))

    def writeQuaternion(self, v):
        self.buffer.extend(struct.pack("<4f", v[0], v[1], v[2], v[3]))


def FloatToString(value):
    return "{:g}".format(value)


def Vector3ToString(vector):
    return "{:g} {:g} {:g}".format(vector[0], vector[1], vector[2])


def Vector4ToString(vector):
    return "{:g} {:g} {:g} {:g}".format(vector[0], vector[1], vector[2], vector[3])


def XmlToPrettyString(elem):
    rough = ET.tostring(elem, 'unicode')
    reparsed = minidom.parseString(rough)
    pretty = reparsed.toprettyxml(indent="\t")
    lines = pretty.split('\n')
    if lines and lines[0].startswith('<?xml'):
        lines = lines[1:]
    return '\n'.join(lines).strip()
