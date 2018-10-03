var fs = require("fs");
var archiver = require("archiver");
var unzip = require("unzip");
var path = require("path");

var greenworks;

if (process.platform === "darwin") {
    if (process.arch === "x64") {
        greenworks = require("./lib/greenworks-osx64");
    } else if (process.arch === "ia32") {
        greenworks = require("./lib/greenworks-osx32");
    }
} else if (process.platform === "win32") {
    if (process.arch === "x64") {
        greenworks = require("./lib/greenworks-win64");
    } else if (process.arch === "ia32") {
        greenworks = require("./lib/greenworks-win32");
    }
} else if (process.platform === "linux") {
    if (process.arch === "x64") {
        greenworks = require("./lib/greenworks-linux64");
    } else if (process.arch === "ia32") {
        greenworks = require("./lib/greenworks-linux32");
    }
}

function error_process(err, errorCallback) {
    if (err && errorCallback) {
        errorCallback(err);
    }
}

// An utility function for publish related APIs.
// It processes remains steps after saving files to Steam Cloud.
function file_share_process(fileName, imageName, nextProcessFunc, errorCallback, progressCallback) {
    if (progressCallback) {
        progressCallback("Completed on saving files on Steam Cloud.");
    }
    greenworks.fileShare(fileName, function() {
        greenworks.fileShare(imageName, function() {
            nextProcessFunc();
        }, function(err) { error_process(err, errorCallback); });
    }, function(err) { error_process(err, errorCallback); });
}

// Publishing user generated content(ugc) to Steam contains following steps:
// 1. Save file and image to Steam Cloud.
// 2. Share the file and image.
// 3. publish the file to workshop.
greenworks.ugcPublish = function(fileName, title, description, imageName, successCallback, errorCallback, progressCallback) {
    var publishFileProcess = function() {
        if (progressCallback) {
            progressCallback("Completed on sharing files.");
        }
        greenworks.publishWorkshopFile(fileName, imageName, title, description,
            function(publishFileID) { successCallback(publishFileID); },
            function(err) { error_process(err, errorCallback); });
    };
    greenworks.saveFilesToCloud([fileName, imageName], function() {
        file_share_process(fileName, imageName, publishFileProcess,
            errorCallback, progressCallback);
    }, function(err) { error_process(err, errorCallback); });
};

// Update publish ugc steps:
// 1. Save new file and image to Steam Cloud.
// 2. Share file and images.
// 3. Update published file.
greenworks.ugcPublishUpdate = function(publishedFileID, fileName, title, description, imageName, successCallback, errorCallback, progressCallback) {
    var updatePublishedFileProcess = function() {
        if (progressCallback) {
            progressCallback("Completed on sharing files.");
        }
        greenworks.updatePublishedWorkshopFile(publishedFileID,
            fileName, imageName, title, description,
            function() { successCallback(); },
            function(err) { error_process(err, errorCallback); });
    };

    greenworks.saveFilesToCloud([fileName, imageName], function() {
        file_share_process(fileName, imageName, updatePublishedFileProcess,
            errorCallback, progressCallback);
    }, function(err) { error_process(err, errorCallback); });
};

// Greenworks Utils APIs implmentation.
greenworks.Utils.move = function(sourceDir, targetDir, successCallback, errorCallback) {
    fs.rename(sourceDir, targetDir, function(err) {
        if (err) {
            if (errorCallback) {
                errorCallback(err);
            }
            return;
        }
        if (successCallback) {
            successCallback();
        }
    });
};

greenworks.Utils.createArchive = function(zipPath, sourceDir, successCallback, errorCallback) {
    var output = fs.createWriteStream(zipPath);
    var archive = archiver("zip");
    output.on("close", successCallback);

    if (errorCallback) {
        archive.on("error", errorCallback);
    }

    archive.pipe(output);

    // Replace '\' with '/'
    sourceDir = sourceDir.replace(/\\/g, "/");

    // Remove last '/' character.
    if (sourceDir[sourceDir.length - 1] === "/") {
        sourceDir = sourceDir.slice(0, -1);
    }

    archive.bulk([{ expand: true, cwd: path.normalize(sourceDir + "/../"), src: [path.basename(sourceDir) + "/**"], dot: true }]);
    archive.finalize();
};

greenworks.Utils.extractArchive = function(zipFilePath, extractDir, successCallback, errorCallback) {
    var unzipExtractor = unzip.Extract({ path: extractDir });
    if (errorCallback) {
        unzipExtractor.on("error", errorCallback);
    }

    unzipExtractor.on("close", successCallback);
    fs.createReadStream(zipFilePath).pipe(unzipExtractor);
};

// run steam callbacks every 50ms
setInterval(function() {
    greenworks.runCallbacks();
}, 50);

module.exports = greenworks;
