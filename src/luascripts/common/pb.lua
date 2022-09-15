local pb = require "pb"

local filelist = C.UTILS.dir(C.ARGS.scriptdir .. "/proto")
if filelist then
    for _, fname in ipairs(filelist) do
        if string.find(fname, "[%d%w].pb") ~= nil then
            pb.loadfile(fname)
        end
    end
end

--[[
local data = {
    srvid = 1212,
    roomid = 18,
    matchid = 22,
    passwd = "passwd",
    roomtype = 3
}
local bytes = assert(pb.encode("network.cmd.RoomIndexData", data))
C.debug(tostring(pb.tohex(bytes)))
local data2 = assert(pb.decode("network.cmd.RoomIndexData", bytes))
C.debug(require"serpent".block(data2))
-- ]] --
