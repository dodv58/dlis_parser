{
    "targets": [{
        "target_name": "dlis_parser",
        "sources": ["src/main.cc", "../src/dlis.c", "../src/common.c"],
        "include_dirs": [
            "<!(node -e \"require('nan')\")",
            "../src"
        ],
        "libraries": [
            "-lbinn", "-lzmq"
        ]
    }]
}
