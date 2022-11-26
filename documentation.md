## Swiss Cheese

Description here

### Table of Contents

> [Command Line Flags](#command-line-flags)  
> [Examples](#examples)  

---
### Command Line Flags

#### \-watch
> Watches the input file for any changes (based on file modified date, if applicable) and then generates output.

#### \-always
> When enabled, the processor ignores the file's modified date and enters an infinite loop in order to perform constant output.

---
### Examples
Example #1
```scss
/*input*/
.body {
	width: 50px;
	.nav
	{
		height: 25px;
	}
}
```
```css
/*output*/
.body
{
	width: 50px;
}

.body .nav
{
	height: 25px;
}
```

Example #2
```scss
/*input*/
$body-width: 25px;
.body
{
	width: $body-width;
	.nav
	{
		height: 25px;
	}
}
```
```css
/*output*/
.body
{
	width: 25px;
}

.body .nav
{
	height: 25px;
}
```
