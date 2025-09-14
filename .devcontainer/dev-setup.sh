#!/bin/bash
# Development setup script for nanopb devcontainer

set -e

echo "🚀 Setting up nanopb development environment..."

# Compile validate.proto if it hasn't been compiled yet
if [ ! -f "/workspace/generator/proto/validate_pb2.py" ]; then
    echo "📦 Compiling validate.proto..."
    cd /workspace/generator/proto
    if command -v protoc &> /dev/null; then
        protoc --python_out=. validate.proto
    else
        echo "⚠️  protoc not found, skipping validate.proto compilation"
    fi
    cd /workspace
fi

# Create a simple test to verify the setup
echo "✅ Verifying installation..."
python3 -c "
import sys
sys.path.insert(0, '/workspace/generator')
try:
    import nanopb_pb2
    print('✅ nanopb_pb2 module loaded successfully')
except ImportError as e:
    print('❌ Failed to load nanopb_pb2:', e)

try:
    import nanopb_generator
    print('✅ nanopb_generator module loaded successfully')
except ImportError as e:
    print('❌ Failed to load nanopb_generator:', e)

try:
    import nanopb_validator
    print('✅ nanopb_validator module loaded successfully')
except ImportError as e:
    print('❌ Failed to load nanopb_validator:', e)
"

echo ""
echo "🎉 Development environment is ready!"
echo ""
echo "Quick start commands:"
echo "  cd examples/simple && make        # Build the simple example"
echo "  cd tests && scons                 # Run all tests"
echo "  ./generator/nanopb_generator.py --help  # Show generator help"
echo ""
