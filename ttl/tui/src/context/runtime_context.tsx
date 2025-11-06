import React, { createContext, useContext } from 'react';
import type { Executor } from '@/runtime/executor.js';

type Runtime = {
	readonly executors: ReadonlyMap<string, Executor>;
};

const RuntimeContext = createContext<Runtime | null>(null);

export const RuntimeProvider: React.FC<{ runtime: Runtime; children: React.ReactNode }> = ({ runtime, children }) => {
	return <RuntimeContext.Provider value={runtime}>{children}</RuntimeContext.Provider>;
};

export const useRuntime = (): Runtime => {
	const context = useContext(RuntimeContext);
	if (!context) {
		throw new Error('useRuntime must be used within RuntimeProvider');
	}
	return context;
};

export const useExecutor = (schema: string): Executor => {
	const runtime = useRuntime();
	const executor = runtime.executors.get(schema);
	if (!executor) {
		throw new Error(`No executor found for schema: ${schema}`);
	}
	return executor;
};
