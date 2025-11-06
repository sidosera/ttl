import { useEffect, useState } from 'react';
import { useStdout } from 'ink';

export function useTerminalSpace() {
	const { stdout } = useStdout();
	const [size, setSize] = useState(() => ({
		width: stdout?.columns ?? process.stdout?.columns,
		height: stdout?.rows ?? process.stdout?.rows
	}));

	useEffect(() => {
		if (!stdout) {
			return;
		}

		const handleResize = () => {
			setSize({ width: stdout.columns, height: stdout.rows });
		};

		stdout.on('resize', handleResize);
		return () => {
			stdout.off('resize', handleResize);
		};
	}, [stdout]);

	return size;
}
