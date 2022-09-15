C = CAPI

local function init()
    local ldir = {"/common"}
    for _, v in ipairs(ldir) do
        local filelist = C.UTILS.dir(C.ARGS.scriptdir .. v)
        if filelist then
            for _, fname in ipairs(filelist) do
                if string.find(fname, "[%d%w].lua$") ~= nil then
                    dofile(fname)
                end
            end
        end
    end
end

collectgarbage("collect")
init()

local function init_service()
    local server = string.format("%s/services/%d", C.ARGS.scriptdir, C.ARGS.sid >> 16)
    local filelist = C.UTILS.dir(server)
    if filelist then
        for _, fname in ipairs(filelist) do
            if string.find(fname, "^[%w]+[%w_]*%.lua$") ~= nil then
                dofile(fname)
            end
        end
    end
end

--init_service()

local json = require "json"
local date = require "date"
local st = {
    a = 1,
    b = "string",
    c = "add field"
}
local function test_timer(arg)
    arg.a = 100
    C.LOG.debug(string.format("test_timer %s", json.encode(arg)))
end
t0 = t0 or C.TIMER.create()
local function test()
    print(C.RAND.rand(), C.RAND.rand_between(1, 10))
    C.TIMER.tick(t0, 1, 3000, test_timer, st)
end
-- test()

function Dispatch(uid, maincmd, subcmd, taskid, body)
    local resp = {
        a = 1,
        b = "string",
        c = "add field"
    }
    return json.encode(resp)
end
