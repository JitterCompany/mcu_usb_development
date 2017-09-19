import hashlib
import numpy as np
import struct
import json
import os
import datetime
from collections import namedtuple

def hash_serial(serial):
    raw = hashlib.sha1(bytes(serial, 'utf-8')).hexdigest()[:12]
    return raw[:4] + '-' + raw[4:8] + '-' + raw[8:]

def parse_string_of(T, s):
     return [T(c) for c in s.split()]

def parse_float_arrays(bytes):
    
    fmt = '<f4'
    data = np.frombuffer(bytes, dtype=fmt)
    data = data.reshape(len(data)//3,3).transpose()

    return data

def generate_timestamps(N, fs):
    base = datetime.datetime.now()
    # create array of timestamps by counting back with steps of 1/fs seconds
    arr = np.array(
            [base + datetime.timedelta(seconds=-(i/fs)) for i in range(N)])
    # return reversed array so we count up
    return arr[::-1]


