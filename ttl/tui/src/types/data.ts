export type DataPrimitive = number | string | boolean | null;

export type DataRow = Record<string, DataPrimitive> & {
	// We expect timestamp columns to be numeric unix epoch seconds.
	ts?: number;
};

export type DataFrame = {
	columns: string[];
	rows: DataRow[];
};
