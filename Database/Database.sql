CREATE TABLE Nominals(
	aggregatorID VARCHAR(50) NOT NULL,
	deviceID INT NOT NULL,
	nomTemperature INT,
	nomOxygen INT,
	CONSTRAINT PK_Nominals PRIMARY KEY (aggregatorID,deviceID)
);

CREATE TABLE CollectorData(
	aggregatorID VARCHAR(50) NOT NULL,
	deviceID INT NOT NULL,
	temperature INT NOT NULL,
	oxygen INT NOT NULL,
	powerState INT NOT NULL,
	collectedTime DATETIME NOT NULL,
	CONSTRAINT PK_CollectorData PRIMARY KEY (aggregatorID,deviceID,collectedTime)
);
GO

CREATE LOGIN ReadUser WITH PASSWORD = 'EKmJcfVCPaX&8wfh';
GO

create user ReadUser for login ReadUser;
GO

Grant select, update, insert on CollectorData to ReadUser;
Grant select, update, insert on Nominals to ReadUser;
GO