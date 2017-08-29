#!/usr/bin/env python

import sys, os
import hashlib

def hash_binary(filename):

    try:
        with open(filename, 'rb') as f:
            data = f.read()
            return hashlib.sha256(data).digest()
    except:
        return None
    return None


def append_binary(filename, bytes_to_append):

    result = False
    with open(filename, 'r+b') as f:
        try:
            # seek to end of file and append bytes
            f.seek(0, os.SEEK_END)
            f.write(bytes_to_append)
            result = True
        except:
            result = False
    return result


def verify_binary(filename):

    try:
        with open(filename, 'rb') as f:
            data = f.read()
           
            # check if a valid footer exists
            footer = data[-64:]
            if not footer:
                return None
            if len(footer) is not 64:
                return None
            if not footer.startswith(b"3853:sha256"):
                return None

            # the file was signed before: check the hash

            ref_hash = footer[32:]
            calculated = hashlib.sha256(data[:-64])
            if calculated.digest() == ref_hash:
                # match: return hexadecimal hash value
                return calculated.hexdigest()
            else:
                # no match: file corrupt
                print(b"ref=" + ref_hash)
                print(b"calc=" + calculated.digest())
                return False
    except:
        return None
    return None




def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


def do_verify(filename):
    verify_result = verify_binary(filename)
    if verify_result is not None:
        if verify_result is False:
            eprint("File '{}' appears signed but does not match!".format(filename))
            sys.exit(os.EX_DATAERR)
        else:
            print(verify_result)
            return True 
    return False


def do_sign(filename):
    result_hash = hash_binary(filename)
    if not result_hash:
        eprint("Failed to hash file '{}'".format(filename))
        sys.exit(os.EX_OSFILE)

    # end file with 64 bytes:
    # - starts with the string "3853:sha256"
    # - ends with the 32 sha256 signature bytes
    to_append = b"3853:sha256"
    to_append+= b"\x00" * (32 - len(to_append))
    to_append+= result_hash

    return append_binary(filename, to_append)


if __name__ == "__main__":

    if len(sys.argv) <= 1:
        eprint("Usage: {} <file-to-sign>".format(sys.argv[0]))
        eprint("Usage: ")
        eprint("Usage: returns hexadecimal hash value to stdout on success")
        sys.exit(os.EX_USAGE)

    filename = sys.argv[1]
    
    # append signature if not already done
    if not do_verify(filename):
        if not do_sign(filename):
            eprint("Failed to append hash to file '{}'".format(filename))
            sys.exit(os.EX_OSFILE)

        if not do_verify(filename):
            eprint("Failed to update file '{}'".format(filename))
            sys.exit(os.EX_SOFTWARE)
    
    # at this point the file is verified to have a correct signature
    sys.exit(0)

