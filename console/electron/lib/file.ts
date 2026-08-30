import fs from 'fs/promises';
import {app} from 'electron';
import path from 'path';

async function getAppFilePath(fileName: string) {
    try {
        const docsPath = app.getPath('documents');
        const appFolder = path.join(docsPath, 'BLDC');
        await fs.mkdir(appFolder, { recursive: true });
        return path.join(appFolder, fileName);
    } catch {
        throw new Error('Failed to create app directory: '+ fileName);
    }
}


export async function saveFile(data: ArrayBuffer, filePath: string): Promise<void> {
  // try {
    // Convert ArrayBuffer back to Buffer for Node.js, Blob isn't well handled across IPC boundaries,
		// so instead we convert Blob -> ArrayBuffer in ui and then ArrayBuffer -> Buffer in the main process.
    const buffer = Buffer.from(data);
    await fs.writeFile(await getAppFilePath(filePath), buffer);
    return Promise.resolve();
	// } catch {
    // throw new Error('Failed to save file: '+ filePath);
		// throw error
	// }
}