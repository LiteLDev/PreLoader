#pragma once

#include <PDB.h>
#include <PDB_DBIStream.h>
#include <PDB_InfoStream.h>
#include <PDB_RawFile.h>

#include "Logger.h"

namespace pl::utils {

inline bool handleError(PDB::ErrorCode errorCode) {
    switch (errorCode) {
    case PDB::ErrorCode::Success:
        return false;

    case PDB::ErrorCode::InvalidSuperBlock:
        Error("Invalid Superblock");
        return true;

    case PDB::ErrorCode::InvalidFreeBlockMap:
        Error("Invalid free block map");
        return true;

    case PDB::ErrorCode::InvalidSignature:
        Error("Invalid stream signature");
        return true;

    case PDB::ErrorCode::InvalidStreamIndex:
        Error("Invalid stream index");
        return true;

    case PDB::ErrorCode::UnknownVersion:
        Error("Unknown version");
        return true;

    case PDB::ErrorCode::InvalidStream:
        Error("Invalid stream");
        return true;
    }
    return true;
}

inline bool checkValidDBIStreams(const PDB::RawFile& rawPdbFile, const PDB::DBIStream& dbiStream) {
    if (handleError(dbiStream.HasValidImageSectionStream(rawPdbFile))) return false;
    if (handleError(dbiStream.HasValidPublicSymbolStream(rawPdbFile))) return false;
    if (handleError(dbiStream.HasValidGlobalSymbolStream(rawPdbFile))) return false;
    if (handleError(dbiStream.HasValidSectionContributionStream(rawPdbFile))) return false;
    return true;
}
} // namespace pl::utils
