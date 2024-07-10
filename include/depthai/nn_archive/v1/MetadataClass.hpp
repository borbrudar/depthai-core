#pragma once

#include <string>
#include <optional>

namespace dai {
namespace nn_archive {
namespace v1 {
    enum class DataType : int;
}
}
}

namespace dai {
namespace nn_archive {
namespace v1 {
    /**
     * Metadata object defining the model metadata.
     *
     * Represents metadata of a model.
     *
     * @type name: str
     * @ivar name: Name of the model.
     * @type path: str
     * @ivar path: Relative path to the model executable.
     */


    /**
     * Metadata object defining the model metadata.
     *
     * Represents metadata of a model.
     *
     * @type name: str
     * @ivar name: Name of the model.
     * @type path: str
     * @ivar path: Relative path to the model executable.
     */
    struct MetadataClass {
        /**
         * Name of the model.
         */
        std::string name;
        /**
         * Relative path to the model executable.
         */
        std::string path;
        /**
         * Precision of the model weights.
         */
        std::optional<DataType> precision;
    };
}
}
}
