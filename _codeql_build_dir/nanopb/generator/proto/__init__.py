'''This file dynamically builds the proto definitions for Python.'''
from __future__ import absolute_import

import os
import os.path
import sys
import tempfile
import shutil
import traceback
from tempfile import TemporaryDirectory
from ._utils import has_grpcio_protoc, invoke_protoc, print_versions

def build_nanopb_proto(protosrc, dirname):
    '''Try to build one or more .proto files for python-protobuf.
    protosrc can be a single path or list/tuple of paths.
    Returns True if successful.
    '''

    # Support both legacy string input and new iterable input for multiple protos
    if isinstance(protosrc, (list, tuple)):
        sources = list(protosrc)
    else:
        sources = [protosrc]

    cmd = ["protoc", "--python_out={}".format(dirname)] + sources + ["-I={}".format(dirname)]

    if has_grpcio_protoc():
        # grpcio-tools has an extra CLI argument
        # from grpc.tools.protoc __main__ invocation.
        cmd.append("-I={}".format(_utils.get_grpc_tools_proto_path()))

    try:
        invoke_protoc(argv=cmd)
    except:
        sys.stderr.write("Failed to build nanopb_pb2.py: " + ' '.join(cmd) + "\n")
        sys.stderr.write(traceback.format_exc() + "\n")
        return False

    return True

def load_nanopb_pb2():
    # To work, the generator needs python-protobuf built version of nanopb.proto.
    # There are three methods to provide this:
    #
    # 1) Load a previously generated generator/proto/nanopb_pb2.py
    # 2) Use protoc to build it and store it permanently generator/proto/nanopb_pb2.py
    # 3) Use protoc to build it, but store only temporarily in system-wide temp folder
    #
    # By default these are tried in numeric order.
    # If NANOPB_PB2_TEMP_DIR environment variable is defined, the 2) is skipped.
    # If the value of the $NANOPB_PB2_TEMP_DIR exists as a directory, it is used instead
    # of system temp folder.

    tmpdir = os.getenv("NANOPB_PB2_TEMP_DIR")
    temporary_only = (tmpdir is not None)
    dirname = os.path.dirname(__file__)
    protosrc = os.path.join(dirname, "nanopb.proto")
    protodst = os.path.join(dirname, "nanopb_pb2.py")
    validatesrc = os.path.join(dirname, "validate.proto")
    validatedst = os.path.join(dirname, "validate_pb2.py")

    if tmpdir is not None and not os.path.isdir(tmpdir):
        tmpdir = None # Use system-wide temp dir

    no_rebuild = bool(int(os.getenv("NANOPB_PB2_NO_REBUILD", default = 0)))
    if bool(no_rebuild):
        # Don't attempt to autogenerate nanopb_pb2.py, external build rules
        # should have already done so.
        import nanopb_pb2 as nanopb_pb2_mod
        return nanopb_pb2_mod

    if os.path.isfile(protosrc):
        src_date = os.path.getmtime(protosrc)
        validate_up_to_date = (not os.path.isfile(validatesrc)) or \
                               (os.path.isfile(validatedst) and os.path.getmtime(validatedst) >= os.path.getmtime(validatesrc))
        if os.path.isfile(protodst) and os.path.getmtime(protodst) >= src_date and validate_up_to_date:
            try:
                from . import nanopb_pb2 as nanopb_pb2_mod
                # If validate.proto exists but not built (older installs), attempt lazy import to see if it's present
                if os.path.isfile(validatesrc) and not os.path.isfile(validatedst):
                    # Trigger build of validate.proto only
                    build_nanopb_proto([validatesrc], dirname)
                return nanopb_pb2_mod
            except Exception as e:
                sys.stderr.write("Failed to import nanopb_pb2.py: " + str(e) + "\n"
                                 "Will automatically attempt to rebuild this.\n"
                                 "Verify that python-protobuf and protoc versions match.\n")
                print_versions()

    # Try to rebuild into generator/proto directory (nanopb.proto + validate.proto if available)
    if not temporary_only:
        sources = [protosrc]
        if os.path.isfile(validatesrc):
            sources.append(validatesrc)
        build_nanopb_proto(sources, dirname)

        try:
            from . import nanopb_pb2 as nanopb_pb2_mod
            return nanopb_pb2_mod
        except:
            sys.stderr.write("Failed to import generator/proto/nanopb_pb2.py:\n")
            sys.stderr.write(traceback.format_exc() + "\n")

    # Try to rebuild into temporary directory
    with TemporaryDirectory(prefix = 'nanopb-', dir = tmpdir) as protodir:
        sources = [protosrc]
        if os.path.isfile(validatesrc):
            sources.append(validatesrc)
        build_nanopb_proto(sources, protodir)

        if protodir not in sys.path:
            sys.path.insert(0, protodir)

        try:
            import nanopb_pb2 as nanopb_pb2_mod
            return nanopb_pb2_mod
        except:
            sys.stderr.write("Failed to import %s/nanopb_pb2.py:\n" % protodir)
            sys.stderr.write(traceback.format_exc() + "\n")

    # If everything fails
    sys.stderr.write("\n\nGenerating nanopb_pb2.py failed.\n")
    sys.stderr.write("Make sure that a protoc generator is available and matches python-protobuf version.\n")
    print_versions()
    sys.exit(1)

