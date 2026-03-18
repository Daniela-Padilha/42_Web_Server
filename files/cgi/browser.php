<?php
echo "Content-Type: text/html\r\n\r\n";

echo "<html><body>";
echo "<h1>PHP CGI is working</h1>";

if ($_SERVER["REQUEST_METHOD"] === "GET")
{
	echo "<p>GET request received</p>";
	if (isset($_GET["name"]))
		echo "<p>Name: " . htmlspecialchars($_GET["name"]) . "</p>";
}

if ($_SERVER["REQUEST_METHOD"] === "POST")
{
	echo "<p>POST request received</p>";
	if (isset($_POST["username"]))
		echo "<p>Username: " . htmlspecialchars($_POST["username"]) . "</p>";
}

echo "</body></html>";
?>