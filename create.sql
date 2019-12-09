START TRANSACTION;
CREATE TYPE positions AS ENUM ('PROGRAMMER','TESTER');

CREATE TYPE statuss AS ENUM ('OPEN','DOING','CLOSE');
CREATE TABLE Employ(
    ID SERIAL PRIMARY KEY,
    First_name VARCHAR (255),
    Last_name VARCHAR (255),
    Position positions
);

CREATE TABLE Todo(
    ID SERIAL PRIMARY KEY,
    ID_statement INTEGER REFERENCES Employ(ID) ON DELETE SET NULL ON UPDATE CASCADE,
    ID_responsible INTEGER REFERENCES Employ(ID) ON DELETE SET NULL ON UPDATE CASCADE,
    Name varchar(255),
    Status statuss,
    Comment VARCHAR(500)
);

COMMIT;
