<html><head><style>

@keyframes fadeinout {
	0% { opacity:1;	}
	45% { opacity:1; }
	55% { opacity:0; }
	100% { opacity:0; }
}

#bigimgcontainer img.top {
animation-name: fadeinout;
animation-timing-function: linear;
animation-iteration-count: infinite;
animation-duration: 3s;
animation-direction: alternate;
}

#bigimgcontainer {
  position:relative;
  height:80%;
  width:80%;
  margin:0 auto;
  overflow: hidden;
}

#bigimgcontainer img {
  position:absolute;
  left:0;
}

#bigimgcontainer img.top:hover {
 opacity:0;
 -webkit-transition: opacity 1s ease-in-out;
 -moz-transition: opacity 1s ease-in-out;
 -o-transition: opacity 1s ease-in-out;
 transition: opacity 1s ease-in-out;
}

.grid-container {
	display: grid;
	/* grid-gap: 10px;
	padding: 10px; */
	grid-template-columns: 1fr;
	grid-template-rows:  1fr 250px;
	overflow: hidden;
}

.t{
	width: 180px;
	height: 180px;
	border: 0;
	object-fit: cover;
	background-position: -150px -150px; 
	background-size: 600px 600px;
	background-color: #E4E4E4;
	border-radius:20px;
	margin: 5px;
	transition: background-image 0.35s ease-in-out,background-color 0.35s ease-in-out;
	float: left;
}
.bigimg{
	display:block;
	width: auto;
	height: 100%;
	border: 0;
	margin: 5px;
}
.panel{
	width: 780px;
	height: 100%;
	overflow: scroll;
	margin:0;
	position:relative;
	padding:0;
	background-color:#CCC;
	border-radius:0;
	float: left;
}
.t.sel,#show{
	background-color: #FFF;
}
img{
	border:0; padding:0;bottom:0;
}
img.histo{
	width:25%;
	height:25%;
	position:absolute;
}

img.get{
	position:absolute;
	width:80px;
	/* //height:49px; */
	top:20px;
	cursor:pointer;
	z-index: 3;
}

img.get.input{ right:120px; }
img.get.output{ right:20px; }

#voroArea{ left:0;}
#flatness{ left:25%;}
#torsion{ left:50%;}
#edgeLen{ left:75%;}

body{
	background-color:#CCC;
	text-align:center;
	padding:0;
	margin:0;
	overflow: hidden;
}


#show{
	width:  calc(100% - 780px);
	height: 99%;
	position:relative;
	float: left;
	visibility:hidden;
}

#histocontainer{
	width: 100%;
	height: 100%;
}

<?php
$dir = dirname(__FILE__);
$files = array_filter(glob('*'), 'is_dir');

$n = 0;
foreach ($files as $file) {
	$ext =  pathinfo($file, PATHINFO_EXTENSION) ;
	if ($ext!="") continue;
	if ($file[0]==".") continue;
	//$name =  str_replace(   ".jpg" , "", $file);
	//$search =  str_replace(   "-" , "+", $name,);
	//$search =  str_replace(   " " , "+", $search);
	//if (($ext=="jpg")||($ext=="png")||($ext=="jpeg"))
	//else if (($ext=="php")||($ext=="")) {} 
	//else 
	// print ".a$n       { background-image:url('$file/$file.obj.jpg');}".PHP_EOL;
	// print ".a$n:hover,.a$n.b{ background-image:url('".$file."/".$file."_rem_p0_0_quadrangulation_smooth.obj.jpg');}".PHP_EOL;
	$n++;
}
?>
</style>
</head>
<body>
<div class="panel">
<?php
for ($x = 0; $x < $n; $x++) {
	print "	<img class='t a$x' onclick='show($x)' width='170px' height='170px' loading='lazy' src='$files[$x]/$files[$x].obj.jpg'>".PHP_EOL;
}?>
</div>
<div id="show">
			<a id="input" >
		<img class="get input" title="Download input for this dataset" src="./download_input.png">
		</a>
		<a id="output" >
		<img class="get output" title="Download output for this dataset" src="./download_output.png">
		</a>
<div class="gridcontainer">
<!-- <div  id="bigimgcontainer" class="imgarea"> -->
	<div  id="bigimgcontainer">
		<img id="showimgbottom" class="bigimg bottom">
		<img id="showimgtop" class="bigimg top"/>
	</div>
<!-- <div class="histoarea"> -->
	<div id="histocontainer">
		<img id="edgeLen" loading="lazy" class="histo" title="Edge Length distribution" src="bimba/edgeLenHistogram.png">
		<img id="flatness" loading="lazy" class="histo" title="Flatness distribution" src="bimba/flatnessHistogram.png">
		<img id="torsion" loading="lazy" class="histo" title="Torsion distribution" src="bimba/torsionHistogram.png">
		<img id="voroArea" loading="lazy" class="histo" title="Voronoi Area distribution" src="bimba/voroAreaHistogram.png">
	</div>
</div>
</body>
<script>
d = document.getElementById("show");
var sel = null;
var b = true;
var files = [
	<?php
	$dir = dirname(__FILE__);
	$files = scandir($dir);
	$n = 0;
	foreach ($files as $file) {
		$ext =  pathinfo($file, PATHINFO_EXTENSION) ;
		if ($ext!="") continue;
		if ($file[0]==".") continue;
		//$name =  str_replace(   ".jpg" , "", $file);
		//$search =  str_replace(   "-" , "+", $name,);
		//$search =  str_replace(   " " , "+", $search);
		//if (($ext=="jpg")||($ext=="png")||($ext=="jpeg"))
		//else if (($ext=="php")||($ext=="")) {} 
		//else 
		echo "'$file', ";
	}
	?>];

	show = function(a){
		if (sel!=null) sel.classList.remove("sel");
		sel = document.getElementsByClassName("a"+a)[0];
		sel.classList.add("sel");
		d.className = "";
		d.classList.add("a"+a);
		histo = ["edgeLen","flatness","torsion","voroArea"];
		for (var i=0; i<4; i++) {
			el = document.getElementById(histo[i]);
			el.src = files[a]+"/"+histo[i]+"Histogram.png";
		}
		document.getElementById("show").style.visibility = "visible";
		document.getElementById("showimgbottom").src = files[a]+"/"+files[a]+"_rem_p0_0_quadrangulation_smooth.obj.jpg";
		document.getElementById("showimgtop").src = files[a]+"/"+files[a]+".obj.jpg";
		document.getElementById("output").href = files[a]+"/"+files[a]+"_rem_p0_0_quadrangulation_smooth.zip";
		document.getElementById("input").href = files[a]+"/"+files[a]+".zip";
		
	}
	toggle = function(){ if (b) d.classList.add("b"); else d.classList.remove("b"); b=!b; }
	setInterval( toggle ,1000);
	</script>
	</html>