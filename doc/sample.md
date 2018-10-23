# Solution

```c
solution("cube") {
    cproj("core") {
        type = "executable|shared_lib|framework|static_lib"
        bundle_id = "com.tencent.nextstudio" // for ios and mac
        link_dirs = ["../../sdk/lib"]
        srcs = ["**.c"]
    }
    csproj("age_core") {

    }
    rustproj("nxt_renderer") {
        cargo_root = "xxx.toml" 
    }
    javaproj("standalone") {
        type = "lib"
    }
    apkproj("android") {
        platform = "android"
    }
}
```