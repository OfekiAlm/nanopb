# Service Handler Implementation for nanopb_generator.py

## Overview
Enhanced `nanopb_generator.py` to automatically generate packet filter functions for protobuf services. The generator now creates two public functions that can decode and validate network packets based on service definitions.

## Implementation Details

### 1. Service Class (Lines 1741-1789)
- Created a new `Service` class similar to the existing `Message` and `Enum` classes
- Parses service definitions from proto descriptors
- Tracks RPC methods with their input/output message types
- Provides methods to get input, output, and all message types used in the service

### 2. Service Iteration (Lines 1791-1800)
- Added `iterate_services()` function to recursively find all services in a proto file
- Similar pattern to `iterate_messages()` and `iterate_extensions()`

### 3. ProtoFile Integration (Lines 2091-2099)
- Modified the `ProtoFile.parse()` method to parse services
- Services are now collected alongside messages, enums, and extensions
- Stored in `self.services` list

### 4. Filter Function Generation

#### Function Declarations (Lines 2537-2541, in generate_header)
Generated in the .pb.h file:
```c
int filter_udp(void *ctx, uint8_t *packet, size_t packet_size);
int filter_tcp(void *ctx, uint8_t *packet, size_t packet_size, bool is_to_server);
```

#### Function Implementations (Lines 2532-2677, in generate_source)
Generated in the .pb.c file:

**Helper Function:**
- `validate_message()`: Validates a decoded message using the generated validation functions
- Conditionally compiled with `#ifdef PB_ENABLE_VALIDATION`
- Includes the validation header when validation is enabled
- Calls `pb_validate_<MessageType>()` functions for each message type

**filter_udp():**
- Tries to decode the packet as each possible message type used in services
- For each successful decode, validates the message
- Returns 1 if packet is valid, 0 otherwise
- Direction-agnostic (suitable for UDP where direction may be unknown)

**filter_tcp():**
- Direction-aware packet filtering
- When `is_to_server == true`: tries to decode as RPC input (request) messages
- When `is_to_server == false`: tries to decode as RPC output (response) messages
- Validates decoded messages before accepting
- Returns 1 for valid packets, 0 otherwise

### 5. Message Type Mapping (Lines 2541-2563)
- Maps protobuf type names (e.g., `.chat.ClientMessage`) to generated C struct names (e.g., `chat_ClientMessage`)
- Handles package prefixes correctly
- Separates input and output message types for TCP filtering

## Usage Example

### Proto File (chat.proto)
```protobuf
service ChatService {
    rpc Chat(stream ClientMessage) returns (stream ServerMessage);
    rpc GetServerStatus(StatusRequest) returns (StatusResponse);
}
```

### Generated Functions

The generator creates two public API functions:

```c
// For UDP: tries all message types
int result = filter_udp(NULL, packet_buffer, packet_length);

// For TCP: direction-aware filtering  
int result = filter_tcp(NULL, packet_buffer, packet_length, is_to_server);
```

Both functions:
1. Decode the packet using `pb_decode()`
2. Validate the decoded message (if PB_ENABLE_VALIDATION is defined)
3. Return 1 for valid packets, 0 for invalid/unknown packets

## Integration with PB_BIND

The filter functions work alongside the existing `PB_BIND()` macro:
- They use the same `pb_msgdesc_t` structures generated for each message
- Compatible with all nanopb encoding/decoding features
- Can be integrated into network stacks for automatic packet filtering

## Benefits

1. **Automatic Generation**: Filter functions are generated automatically from service definitions
2. **Type Safety**: Uses the generated message types and descriptors
3. **Validation Support**: Integrates with nanopb validation when enabled
4. **Direction Awareness**: TCP filter understands request vs response messages
5. **Zero Manual Code**: No need to manually write decode/validate logic for each message type

## Testing

Generated code for chat.proto successfully creates:
- `filter_udp()` function that handles all 10 message types from 2 services
- `filter_tcp()` function with separate logic for 5 request types and 5 response types
- Validation integration for all message types with field-level constraint checking

## Files Modified

- `/workspace/generator/nanopb_generator.py`: Main implementation (added ~200 lines)
- Generated files from chat.proto:
  - `chat.pb.h`: Contains function declarations
  - `chat.pb.c`: Contains function implementations with validation logic
