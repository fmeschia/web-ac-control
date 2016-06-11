create database temp;

create user 'temp'@'localhost' identified by 'password';

grant all privileges on *.* to 'temp'@'localhost';

flush privileges;

create table temperature ( id mediumint not null auto_increment, date DATETIME not null, temp FLOAT, PRIMARY KEY (id) ) ENGINE=InnoDb;

create table temperature_history ( id mediumint not null auto_increment, date DATETIME not null, temp FLOAT, PRIMARY KEY (id) ) ENGINE=InnoDb;