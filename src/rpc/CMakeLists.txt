set(RPC_SRCS
rpc.h
rpc.cc
rpc_client.h
rpc_client.cc
rpc_server.h
rpc_server.cc)
source_group(rpc FILES ${RPC_SRCS})
add_library(rpc ${RPC_SRCS})
set_target_properties(rpc PROPERTIES FOLDER "rpc")