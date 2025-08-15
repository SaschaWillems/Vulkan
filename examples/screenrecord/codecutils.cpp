#include "codecutils.h"

const char *amErrorString(media_status_t status) {
    /** The requested media operation completed successfully. */
    switch (status) {
        case AMEDIA_OK:
            return "AMEDIA_OK";
        case AMEDIACODEC_ERROR_INSUFFICIENT_RESOURCE:
            return "AMEDIACODEC_ERROR_INSUFFICIENT_RESOURCE";
        case AMEDIACODEC_ERROR_RECLAIMED:
            return "AMEDIACODEC_ERROR_RECLAIMED";
        case AMEDIA_ERROR_UNKNOWN:
            return "AMEDIA_ERROR_UNKNOWN";
        case AMEDIA_ERROR_MALFORMED:
            return "AMEDIA_ERROR_MALFORMED";
        case AMEDIA_ERROR_UNSUPPORTED:
            return "AMEDIA_ERROR_UNSUPPORTED";
        case AMEDIA_ERROR_INVALID_OBJECT:
            return "AMEDIA_ERROR_INVALID_OBJECT";
        case AMEDIA_ERROR_INVALID_PARAMETER:
            return "AMEDIA_ERROR_INVALID_PARAMETER";
        case AMEDIA_ERROR_INVALID_OPERATION:
            return "AMEDIA_ERROR_INVALID_OPERATION";
        case AMEDIA_ERROR_END_OF_STREAM:
            return "AMEDIA_ERROR_END_OF_STREAM";
        case AMEDIA_ERROR_IO:
            return "AMEDIA_ERROR_IO";
        case AMEDIA_ERROR_WOULD_BLOCK:
            return "AMEDIA_ERROR_WOULD_BLOCK";
        case AMEDIA_DRM_ERROR_BASE:
            return "AMEDIA_DRM_ERROR_BASE";
        case AMEDIA_DRM_NOT_PROVISIONED:
            return "AMEDIA_DRM_NOT_PROVISIONED";
        case AMEDIA_DRM_RESOURCE_BUSY:
            return "AMEDIA_DRM_RESOURCE_BUSY";
        case AMEDIA_DRM_DEVICE_REVOKED:
            return "AMEDIA_DRM_DEVICE_REVOKED";
        case AMEDIA_DRM_SHORT_BUFFER:
            return "AMEDIA_DRM_SHORT_BUFFER";
        case AMEDIA_DRM_SESSION_NOT_OPENED:
            return "AMEDIA_DRM_SESSION_NOT_OPENED";
        case AMEDIA_DRM_TAMPER_DETECTED:
            return "AMEDIA_DRM_TAMPER_DETECTED";
        case AMEDIA_DRM_VERIFY_FAILED:
            return "AMEDIA_DRM_VERIFY_FAILED";
        case AMEDIA_DRM_NEED_KEY:
            return "AMEDIA_DRM_NEED_KEY";
        case AMEDIA_DRM_LICENSE_EXPIRED:
            return "AMEDIA_DRM_LICENSE_EXPIRED";
        case AMEDIA_IMGREADER_ERROR_BASE:
            return "AMEDIA_IMGREADER_ERROR_BASE";
        case AMEDIA_IMGREADER_NO_BUFFER_AVAILABLE:
            return "AMEDIA_IMGREADER_NO_BUFFER_AVAILABLE";
        case AMEDIA_IMGREADER_MAX_IMAGES_ACQUIRED:
            return "AMEDIA_IMGREADER_MAX_IMAGES_ACQUIRED";
        case AMEDIA_IMGREADER_CANNOT_LOCK_IMAGE:
            return "AMEDIA_IMGREADER_CANNOT_LOCK_IMAGE";
        case AMEDIA_IMGREADER_CANNOT_UNLOCK_IMAGE:
            return "AMEDIA_IMGREADER_CANNOT_UNLOCK_IMAGE";
        case AMEDIA_IMGREADER_IMAGE_NOT_LOCKED:
            return "AMEDIA_IMGREADER_IMAGE_NOT_LOCKED";
    }
    return "UNKNOWN";

}