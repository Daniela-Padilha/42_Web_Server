<?php
echo "Content-Type: text/html\r\n\r\n";
echo "<html><body>";
echo "<h1>PHP CGI POST test</h1>";
echo "<p>Method: " . $_SERVER['REQUEST_METHOD'] . "</p>";
echo "<p>Data: " . file_get_contents("php://input") . "</p>";
echo "</body></html>";
?>

<!-- curl -i -X POST -d "name=daniela" http://127.0.0.1:8085/cgi/post.php -->