import validateURL from '../helper_funcs/validate_url.ts';

export default class API {
    static ENDPOINT = "http://localhost:3001/"
    static ESTIMATE_ENDPOINT = API.ENDPOINT + "estimate"
    static PROCESS_ENDPOINT = API.ENDPOINT + "process"
    static GET_DOCS_ENDPOINT = API.ENDPOINT + "getDocs"

    static async estimateVideo(videoURL: string, optionalDescription: string) {
        const validate = validateURL(videoURL)
        if(!validate.isValid) return;
        const resp = await fetch(API.ESTIMATE_ENDPOINT, {
            headers: {'Content-Type': 'application/json', "Access-Control-Allow-Origin": "*",},
            method: "POST",
            body: JSON.stringify({video_id: validate.id, optional_description: optionalDescription})
        });
        return await resp.json();
    }

    static async processVideo(videoURL: string, optionalDescription: string) {
        const validate = validateURL(videoURL)
        if(!validate.isValid) return;
        const resp = await fetch(API.PROCESS_ENDPOINT, {
            headers: {'Content-Type': 'application/json', "Access-Control-Allow-Origin": "*",},
            method: "POST",
            body: JSON.stringify({video_id: validate.id, optional_description: optionalDescription})
        });
        return await resp.json();
    }

    static async getVideoDocuments() {
        try {
            const resp = await fetch(API.GET_DOCS_ENDPOINT, {
                headers: {'Content-Type': 'application/json', "Access-Control-Allow-Origin": "*",},
                method: "GET"
            });
            return await resp.json();
        } catch {
            return {};
        }
    }
}