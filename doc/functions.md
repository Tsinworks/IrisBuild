# function

```c
ret = execute("ls /") // without named arg
ret = execute(command = "ls /") // with named args
```

lambda function

```cpp
on_init = () { // lambda function captures current scope
    download_and_extract(predownload_url, "../temp")
    execute("xbgn --build ../")
}
on_prebuild = () {
}
on_post_build = () {
    pack(target)
}
```

# builtin functions

* **zip**(file_list, root_dir, dest_file): compress file_list to single dest zip under root dir. 
* **unzip**(src_path, dst_dir): extract zip file from source path to dest dir.
* **execute**(cmd) -> (code, stdout, stderr): execute command line
* **download**(url, file, proxy): download file to file path


``` c
    download(url = "http://sss.xx/ff.zip", file = "xx.zip")
    unzip("xx.zip", dest_dir)
```