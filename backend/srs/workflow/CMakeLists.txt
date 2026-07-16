target_sources(
    srscpp
    PRIVATE AnalysisHandle.cpp DataMonitor.cpp TaskDiagram.cpp TaskReport.cpp
    PRIVATE
        FILE_SET privateHeaders
            FILES
                AnalysisHandle.hpp
                BaseTask.hpp
                DataMonitor.hpp
                FrameMissMonitor.hpp
                TaskDiagram.hpp
                TaskReport.hpp
)
