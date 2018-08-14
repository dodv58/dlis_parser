{
    "targets": [{
        "target_name": "dlis_parser",
        "sources": ["src/main.cc", "node-addon/src/dlis.cc", "node-addon/src/common.cc"],
        "include_dirs": [
            "<!(node -e \"require('nan')\")",
            "node-addon/src"
        ],
        "libraries": [
            "-lbinn", "-lzmq"
        ]
    }]
}
