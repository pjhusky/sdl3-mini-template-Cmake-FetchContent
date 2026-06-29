include(FetchContent)

# The build is failing on your GitHub runner because your FetchContent configuration is trying to use an SSH repository link (git@github.com:quickjs-ng/quickjs.git).
# While this works perfectly on your local machine (because your computer has your personal SSH private keys registered with GitHub), the GitHub Actions cloud runner does not have your SSH keys. When it tries to handshake via SSH, GitHub denies access.
# Since quickjs-ng/quickjs is a public repository, you do not need SSH authentication to download it.

# HTTPS URLs allow public repositories to be cloned anonymously by anyone (including automated GitHub cloud runners) without requiring SSH key registration or authentication tokens.

FetchContent_Declare(
    quickjs
    GIT_REPOSITORY https://github.com/quickjs-ng/quickjs.git
    GIT_TAG        v0.15.1
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

FetchContent_MakeAvailable( quickjs )

# add_library(quickjs IMPORTED)

# add_library(quickjsb)
# add_library( quickjs::quickjs ALIAS libquickjs )
# add_library( quickjs::quickjs ALIAS quickjs )

# Fix: Alias 'qjs' to your preferred modern scoped namespace
if(TARGET qjs)
    add_library(quickjs::quickjs ALIAS qjs)
endif()

# Map the companion utility library target as well
if(TARGET qjs-libc)
    add_library(quickjs::quickjs-libc ALIAS qjs-libc)
endif()