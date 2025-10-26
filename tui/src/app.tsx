import React, { useMemo } from 'react';
import { Box } from 'ink';
import { Repl } from '@/repl.js';
import { PaneContainer } from '@/pane_container.js';
import { RuntimeProvider } from '@/context/runtime_context.js';
import { CatalogExecutor } from '@/runtime/catalog_executor.js';
import { useTerminalSpace } from '@/hooks/use_terminal_space.js';

export type AppProps = Record<string, never>;

export const App: React.FC<AppProps> = () => {
	const runtime = useMemo(() => {
		const executors = new Map();
		executors.set('catalog', new CatalogExecutor());
		return { executors };
	}, []);

	const { width, height } = useTerminalSpace();

	return (
		<RuntimeProvider runtime={runtime}>
			<Box flexDirection="column" width={width} height={height}>
				<Box flexGrow={1}>
					<PaneContainer />
				</Box>
				<Box flexShrink={0}>
					<Repl />
				</Box>
			</Box>
		</RuntimeProvider>
	);
};
