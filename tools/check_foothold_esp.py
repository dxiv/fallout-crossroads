import struct
import sys

path = sys.argv[1] if len(sys.argv) > 1 else (
    r"C:\Program Files (x86)\Steam\steamapps\common\Fallout New Vegas\Data\CrossroadsFoothold.esp"
)
with open(path, "rb") as f:
    data = f.read()
print("len", len(data))
pos = 0
records = []
while pos + 4 <= len(data):
    typ = data[pos : pos + 4]
    if typ == b"GRUP":
        gsz, label, gtype = struct.unpack_from("<III", data, pos + 4)
        ver = data[pos + 20 : pos + 24]
        print(f"{pos:4d} GRUP size={gsz} label={label:#010x} type={gtype} ver={ver.hex()}")
        if gsz < 24 or pos + gsz > len(data):
            print("BAD GRUP")
            sys.exit(1)
        pos += gsz
        continue
    rsz, flags, fid = struct.unpack_from("<III", data, pos + 4)
    payload = data[pos + 20 : pos + 20 + rsz]
    edid = ""
    k = 0
    while k + 6 <= len(payload):
        st = payload[k : k + 4]
        ssz = struct.unpack_from("<H", payload, k + 4)[0]
        k += 6
        if k + ssz > len(payload):
            break
        if st == b"EDID":
            edid = payload[k : k + ssz].split(b"\x00")[0].decode("latin1", "replace")
            break
        k += ssz
    print(f"{pos:4d} {typ.decode('ascii', 'replace')} id={fid:#010x} sz={rsz} edid={edid!r}")
    records.append(edid)
    pos += 20 + rsz
print("end", pos, "file", len(data), "ok", pos == len(data))
for need in [
    "CR_CapitalArrive",
    "CR_CapitalTransitQuest",
    "CR_CapitalGateMitchell",
    "CR_CapitalMapMarker",
    "CR_CapitalGuideNote",
]:
    print(need, need in records or need.encode() in data)
sys.exit(0 if pos == len(data) else 2)
