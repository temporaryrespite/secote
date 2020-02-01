# secote - set\_color\_temperature

secote is a C program which sets the color temperature to the specified-on-command-line Kelvin or RGB gamma values.  
Works for both X and tty/drm(aka TERM=linux).  
Technically, I made this only for myself...  
TODO: make it easier for others to compile and mention it in readme, for me: `./go`  

secote is based on code from [blugon](https://github.com/jumper149/blugon) v1.11.3

## Example usage:

Set color temperature to 3000 Kelvin:  
`./secote 3000`  
`./secote 1 0.6949030005552019 0.4310480202110507`  
  
If you have [redshift](https://github.com/jonls/redshift/), you don't need secote!  
on X: `redshift -P -O 3000` or `redshift -P -O 3000 -m randr`  
on tty/drm: `redshift -P -O 3000 -m drm`  


## License(s)

Any/all of the following, for maximum freedom:  
CC0, UNLICENSE, MIT, Apache v2, GPL v3  

