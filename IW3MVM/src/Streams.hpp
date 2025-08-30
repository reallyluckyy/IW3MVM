#pragma once

extern void				Write_CameraData(std::string path, bool finish);
extern void				Avidemo_Frame();
extern void				Streams_Frame();

enum class StreamsOrder {
	Prepare,
	Execute,
	Capture,
	Advance,
	Export,
};

enum class RecordingStatus {
	Disabled,
	Running,
	Finish,
	Finished
};

extern StreamsOrder		streamsOrder;
extern RecordingStatus	recordingStatus;
extern std::uint32_t	maxfps_cap;

extern void Reset_Recording();